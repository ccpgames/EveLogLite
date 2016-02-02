#include "qloglitelogger.h"
#include <QCoreApplication>
#include <QtNetwork/QHostInfo>
#include <QDateTime>

namespace
{

enum MessageType
{
    CONNECTION_MESSAGE,
    SIMPLE_MESSAGE,
    LARGE_MESSAGE,
    CONTINUATION_MESSAGE,
    CONTINUATION_END_MESSAGE,
};

template <size_t size>
void fillString(char (&destination)[size], QString source)
{
    QByteArray src = source.toLocal8Bit();

    strncpy(destination, src.data(), src.size());
    if (src.size() < size)
    {
        destination[src.size()] = 0;
    }
    else
    {
        destination[size - 1] = 0;
    }
}

}



QLogLiteLogger::QLogLiteLogger()
    :m_pid(0),
    m_state(Disconnected)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &QLogLiteLogger::connected);
}

void QLogLiteLogger::connectToHost()
{
    connectToHost(QCoreApplication::applicationPid(), QHostInfo::localHostName(), QCoreApplication::applicationFilePath());
}

void QLogLiteLogger::connectToHost(qint64 pid, QString machineName, QString executablePath)
{
    connectToHost("127.0.0.1", pid, machineName, executablePath);
}

void QLogLiteLogger::connectToHost(QString server, qint64 pid, QString machineName, QString executablePath)
{
    m_pid = pid;
    m_machineName = machineName;
    m_executablePath = executablePath;
    m_state = Connecting;
    m_socket->connectToHost(server, 0xCC9);
}

void QLogLiteLogger::disconnnect()
{
    m_state = Disconnected;
    m_socket->disconnectFromHost();
}

bool QLogLiteLogger::isConnected() const
{
    return m_socket->state() == QTcpSocket::ConnectedState;
}

void QLogLiteLogger::setDefaultModule(QString module)
{
    m_module = module;
}

QString QLogLiteLogger::defaultModule() const
{
    return m_module;
}

void QLogLiteLogger::setDefaultChannel(QString channel)
{
    m_channel = channel;
}

QString QLogLiteLogger::defaultChannel() const
{
    return m_channel;
}

void QLogLiteLogger::log(LogSeverity severity, QDateTime timestamp, QString module, QString channel, QString message)
{
    if (m_state == Disconnected)
    {
        return;
    }

    RawLogMessage msg;

    msg.text.timestamp = timestamp.toMSecsSinceEpoch();
    msg.text.severity = severity;
    fillString(msg.text.channel, channel);
    fillString(msg.text.module, module);
    fillString(msg.text.channel, channel);

    QByteArray text = message.toLocal8Bit();
    if (text.size() >= TextMessage::TEXT_SIZE)
    {
        msg.type = LARGE_MESSAGE;
    }
    else
    {
        msg.type = SIMPLE_MESSAGE;
    }
    int offset = 0;
    do
    {
        strncpy(msg.text.message, text.data() + offset, TextMessage::TEXT_SIZE - 1);
        msg.text.message[TextMessage::TEXT_SIZE - 1] = 0;
        offset += TextMessage::TEXT_SIZE - 1;

        if (m_state == Connected)
        {
            m_socket->write(reinterpret_cast<const char*>(&msg), sizeof(msg));
        }
        else
        {
            m_pendingMessages.append(msg);
        }
        if (offset + TextMessage::TEXT_SIZE < text.size())
        {
            msg.type = CONTINUATION_MESSAGE;
        }
        else
        {
            msg.type = CONTINUATION_END_MESSAGE;
        }
    }
    while (offset < text.size());
}

void QLogLiteLogger::log(LogSeverity severity, QString module, QString channel, QString message)
{
    log(severity, QDateTime::currentDateTime(), module, channel, message);
}

void QLogLiteLogger::info(QString message)
{
    log(SEVERITY_INFO, m_module, m_channel, message);
}

void QLogLiteLogger::notice(QString message)
{
    log(SEVERITY_NOTICE, m_module, m_channel, message);
}

void QLogLiteLogger::warn(QString message)
{
    log(SEVERITY_WARN, m_module, m_channel, message);
}

void QLogLiteLogger::error(QString message)
{
    log(SEVERITY_ERR, m_module, m_channel, message);
}

void QLogLiteLogger::flush()
{
    QAbstractSocket::SocketState state = m_socket->state();
    if (state == QAbstractSocket::HostLookupState || state == QAbstractSocket::ConnectingState)
    {
        m_socket->waitForConnected();
    }
    m_socket->waitForBytesWritten();
}

void QLogLiteLogger::connected()
{
    RawLogMessage msg;
    msg.type = CONNECTION_MESSAGE;
    msg.connection.pid = m_pid;
    msg.connection.version = 2;
    fillString(msg.connection.machineName, m_machineName);
    fillString(msg.connection.executablePath, m_executablePath);

    m_socket->write(reinterpret_cast<const char*>(&msg), sizeof(msg));

    for (QVector<RawLogMessage>::iterator it = m_pendingMessages.begin(); it != m_pendingMessages.end(); ++it)
    {
        const RawLogMessage& m = *it;
        m_socket->write(reinterpret_cast<const char*>(&m), sizeof(m));
    }
    m_pendingMessages.clear();
    m_state = Connected;
}
