#include "logmodel.h"
#include <QTcpSocket>
#include <QDateTime>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QHostInfo>
#include <QTimer>
#include <QMessageBox>
#include "logmonitorfilemodel.h"
#include <cmath>

namespace
{

const uint32_t VERSION = 2;

enum MessageType
{
    CONNECTION_MESSAGE,
    SIMPLE_MESSAGE,
    LARGE_MESSAGE,
    CONTINUATION_MESSAGE,
    CONTINUATION_END_MESSAGE,
};

struct ConnectionMessage
{
    static const size_t MESSAGE_MAX_PATH = 260;

    uint32_t version;
    uint64_t pid;
    char machineName[32];
    char executablePath[MESSAGE_MAX_PATH];
};

struct TextMessage
{
    static const size_t TEXT_SIZE = 256;

    uint64_t timestamp;
    uint32_t severity;
    char module[32];
    char channel[32];
    char message[TEXT_SIZE];
};

struct RawLogMessage
{
    uint32_t type;
    union
    {
        ConnectionMessage connection;
        TextMessage text;
    };
};

}

LogModel::RunningCount::RunningCount()
    :m_bin(0),
      m_count(0)
{
    m_bins.resize(int(ceil(5.f / 0.5f)));
}

void LogModel::RunningCount::add()
{
    ++m_bins[m_bin];
    ++m_count;
}

void LogModel::RunningCount::update()
{
    m_bin = (m_bin + 1) % m_bins.size();
    m_count -= m_bins[m_bin];
    m_bins[m_bin] = 0;
}

int LogModel::RunningCount::get() const
{
    return m_count;
}


LogModel::Client::Client()
    :m_socket(nullptr)
{
}

LogModel::Client::Client(QTcpSocket* socket)
    :m_socket(socket)
{
}

uint64_t LogModel::Client::pid() const
{
    if (m_socket)
    {
        return m_socket->property("pid").toULongLong();
    }
    return 0;
}

QString LogModel::Client::path() const
{
    if (m_socket)
    {
        return m_socket->property("executablePath").toString();
    }
    return QString();
}

QString LogModel::Client::machine() const
{
    if (m_socket)
    {
        return m_socket->property("machineName").toString();
    }
    return QString();
}

QTcpSocket* LogModel::Client::socket() const
{
    return m_socket;
}

bool LogModel::Client::operator==(const Client& other) const
{
    return m_socket == other.m_socket;
}


uint qHash(const LogModel::Client& client)
{
    return qHash(client.socket());
}


LogModel::LogModel(QObject* parent)
    :AbstractLogModel(parent),
    m_nextMessage(nullptr),
    m_maxMessages(100000),
    m_serverMode(false)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    m_listening = m_server.listen(QHostAddress::Any, 0xCC9);
    if (m_listening)
    {
        qDebug() << "Server started";
    }
    else
    {
        qDebug() << "Server failed to start";
    }

    m_statistics.error = 0;
    m_statistics.warning = 0;
    m_statistics.notice = 0;
    m_statistics.info = 0;
    m_statistics.clients = 0;

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &LogModel::updateRuningCounts);
    timer->start(500);
}

LogModel::~LogModel()
{

}

const LogModel::Clients& LogModel::clients() const
{
    return m_clients;
}

bool LogModel::clientFromMessage(const LogMessage& message, Client& client)
{
    auto found = std::find_if(m_clients.begin(), m_clients.end(), [&](const Client& client)
    {
        return client.pid() == message.pid && client.machine() == message.machineName &&
                client.path() == message.executablePath;
    });
    if (found == m_clients.end())
    {
        return false;
    }
    client = *found;
    return true;
}

void LogModel::disconnect(const Client& client)
{
    if (!client.socket())
    {
        return;
    }
    client.socket()->disconnectFromHost();
}

bool LogModel::isListening() const
{
    return m_listening;
}

void LogModel::setServerMode(bool serverMode)
{
    m_serverMode = serverMode;
    if (m_serverMode && m_messages.size() >= m_maxMessages)
    {
        autoSave();
        clear();
    }
}

bool LogModel::isInServerMode() const
{
    return m_serverMode;
}

void LogModel::setMaxMessages(int maxMessages)
{
    m_maxMessages = maxMessages;
    if (m_serverMode && m_messages.size() >= m_maxMessages)
    {
        autoSave();
        clear();
    }
}

int LogModel::maxMessages() const
{
    return m_maxMessages;
}

void LogModel::setAutoSaveDirectory(const QString& autoSaveDirectory)
{
    m_autoSaveDirectory = autoSaveDirectory;
}

QString LogModel::autoSaveDirectory() const
{
    return m_autoSaveDirectory;
}

int LogModel::getRunningCount(LogSeverity severity)
{
    return m_runningCounts[severity].get();
}

void LogModel::acceptConnection()
{
    m_statistics.clients++;
    while (auto socket = m_server.nextPendingConnection())
    {
        socket->setProperty("receivedConnectionMessage", false);
        connect(socket, &QTcpSocket::readyRead, this, &LogModel::readMessages);
        connect(socket, &QTcpSocket::disconnected, this, &LogModel::socketDisconnected);
        m_clients.insert(socket);
        emit clientConnected();
    }
}

void LogModel::socketDisconnected()
{
    m_clients.remove(static_cast<QTcpSocket*>(sender()));
    m_statistics.clients--;
    emit clientDisconnected();
}

void LogModel::readMessages()
{
    auto socket = static_cast<QTcpSocket*>(sender());
    RawLogMessage msg;
    int count = m_messages.size();
    while (socket->bytesAvailable() >= qint64(sizeof(msg)))
    {
        socket->read(reinterpret_cast<char*>(&msg), sizeof(msg));
        bool receivedConnectionMessage = socket->property("receivedConnectionMessage").toBool();
        if ((msg.type != CONNECTION_MESSAGE) != receivedConnectionMessage )
        {
            socket->abort();
            return;
        }
        if (msg.type == CONNECTION_MESSAGE)
        {
            if (msg.connection.version > VERSION)
            {
                socket->abort();
                return;
            }
            socket->setProperty("version", msg.connection.version);
            socket->setProperty("receivedConnectionMessage", true);
            socket->setProperty("pid", quint64(msg.connection.pid));
            socket->setProperty("machineName", QString::fromLocal8Bit(msg.connection.machineName));
            socket->setProperty("executablePath", QString::fromLocal8Bit(msg.connection.executablePath));
            continue;
        }

        if (!m_nextMessage)
        {
            m_nextMessage = new LogMessage;
            if (socket->property("version") == 1)
            {
                m_nextMessage->timestamp = QDateTime::fromTime_t(msg.text.timestamp);
            }
            else
            {
                m_nextMessage->timestamp.setMSecsSinceEpoch(msg.text.timestamp);
            }
            m_nextMessage->pid = uint64_t(socket->property("pid").toULongLong());
            m_nextMessage->severity = LogSeverity(msg.text.severity);
            m_nextMessage->machineName = socket->property("machineName").toString();
            m_nextMessage->executablePath = socket->property("executablePath").toString();
            m_nextMessage->module = QString::fromLocal8Bit(msg.text.module);
            m_nextMessage->channel = QString::fromLocal8Bit(msg.text.channel);
        }
        m_receivedText.append(msg.text.message);

        if (msg.type == SIMPLE_MESSAGE || msg.type == CONTINUATION_END_MESSAGE)
        {
            m_nextMessage->message = QString::fromUtf8(m_receivedText.constData());
            m_receivedText.clear();
            addMessage(m_nextMessage);
            m_runningCounts[m_nextMessage->severity].add();
            switch (m_nextMessage->severity)
            {
            case SEVERITY_ERR:
                m_statistics.error++;
                break;
            case SEVERITY_WARN:
                m_statistics.warning++;
                break;
            case SEVERITY_NOTICE:
                m_statistics.notice++;
                break;
            case SEVERITY_INFO:
                m_statistics.info++;
                break;
            default:
                break;
            }
            m_nextMessage = nullptr;
        }
    }
    if (m_messages.size() > count)
    {
        beginInsertRows(QModelIndex(), count, m_messages.size() - 1);
        endInsertRows();
    }
    if (m_serverMode && m_messages.size() >= m_maxMessages)
    {
        if (autoSave())
        {
            clear();
            count = 0;
        }
    }
}

const LogModel::Statistics &LogModel::statistics() const
{
    return m_statistics;
}

bool LogModel::autoSave()
{
    if (!QDir(m_autoSaveDirectory).exists())
    {
        QDir().mkpath(m_autoSaveDirectory);
    }
    auto fi = QString("%1%2%3.%4.lsw").arg(m_autoSaveDirectory, QDir::separator(), QHostInfo::localHostName(), QDateTime::currentDateTime().toString("yyyy-MM-dd_HH.mm.ss"));
    return LogMonitorFileModel::saveModel(this, fi);
}

void LogModel::clear()
{
    if (!m_messages.size())
    {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, m_messages.size() - 1);
    for (int i = 0; i < m_messages.size(); ++i)
    {
        delete m_messages[i];
    }
    m_messages.clear();

    m_statistics.error = 0;
    m_statistics.warning = 0;
    m_statistics.notice = 0;
    m_statistics.info = 0;
    endRemoveRows();
}

void LogModel::disconnectAll()
{
    auto copy = m_clients;
    for (auto it = copy.begin(); it != copy.end(); ++it)
    {
        disconnect(*it);
    }
}

void LogModel::updateRuningCounts()
{
    for (int i = 0; i < SEVERITY_COUNT; ++i)
    {
        m_runningCounts[i].update();
    }
}
