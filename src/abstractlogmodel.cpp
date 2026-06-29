#include "abstractlogmodel.h"
#include <QPixmap>
#include <QTextStream>


AbstractLogModel::AbstractLogModel(QObject* parent)
    :QAbstractTableModel(parent),
      m_breakLines(false),
      m_splitByPids(false),
      m_timestampPrecision(PRECISION_MINUTES),
      m_colorBackground(COLOR_NONE),
      m_colorTheme(THEME_LIGHT)
{
    m_logTypes.resize(SEVERITY_COUNT);
    m_logTypes[SEVERITY_INFO] = QPixmap(":/default/info");
    m_logTypes[SEVERITY_NOTICE] = QPixmap(":/default/notice");
    m_logTypes[SEVERITY_WARN] = QPixmap(":/default/warning");
    m_logTypes[SEVERITY_ERR] = QPixmap(":/default/error");
}

const LogMessage *AbstractLogModel::message(int index) const
{
    return index >= 0 && index < m_messages.size() ? m_messages[index] : nullptr;
}

void AbstractLogModel::setBreakLines(bool breakLines)
{
    if (m_breakLines == breakLines)
    {
        return;
    }
    m_breakLines = breakLines;
    if (m_breakLines)
    {
        explodeMessages();
    }
    else
    {
        collapseMessages();
    }
}

void AbstractLogModel::setColorBackground(LogColorBackground colorBackground)
{
    m_colorBackground = colorBackground;
    refreshColorBackgroundTheme();
}

void AbstractLogModel::setColorTheme(LogColorTheme colorTheme)
{
    m_colorTheme = colorTheme;
    refreshColorBackgroundTheme();

}

void AbstractLogModel::setSplitByPid(bool split)
{
    if (m_splitByPids == split)
    {
        return;
    }
    m_splitByPids = split;
    if (m_pids.size() > 1)
    {
        if (split && m_pids.size() > 1)
        {
            beginInsertColumns(QModelIndex(), 7, 7 + m_pids.size() - 2);
            endInsertColumns();
        }
        else
        {
            beginRemoveColumns(QModelIndex(), 7, 7 + m_pids.size() - 1);
            endRemoveColumns();
        }
    }
}

bool AbstractLogModel::splitByPid() const
{
    return m_splitByPids;
}

void AbstractLogModel::saveAsText(QIODevice* output)
{
    QTextStream s(output);

    auto rows = m_messages.size();

    for (int i = 0; i < rows; ++i)
    {
        auto msg = message(i);
        if (!msg)
        {
            continue;
        }
        switch (msg->severity)
        {
        case SEVERITY_INFO:
            s << "info";
            break;
        case SEVERITY_NOTICE:
            s << "notice";
            break;
        case SEVERITY_WARN:
            s << "warning";
            break;
        case SEVERITY_ERR:
            s << "error";
            break;
        default:
            break;
        }
        for (int j = 0; j < columnCount(); ++j)
        {
            s << "\t";
            s << data(index(i, j)).toString();
        }
        s << Qt::endl;
    }
}

int AbstractLogModel::rowCount(const QModelIndex &) const
{
    return m_messages.size() + 1;
}

int AbstractLogModel::columnCount(const QModelIndex &) const
{
    if (m_splitByPids && m_pids.size() > 1)
    {
        return 6 + m_pids.size();
    }
    return 7;
}

QVariant AbstractLogModel::data(const QModelIndex & index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::BackgroundRole) || index.row() >= m_messages.size())
    {
        return QVariant();
    }
    auto& message = *m_messages[index.row()];
    if (role == Qt::BackgroundRole)
    {
        if (m_colorBackground == COLOR_NONE)
        {
            return QVariant();
        }
        else
        {
            switch (message.severity)
            {
            case SEVERITY_ERR:
                return m_colorTheme == THEME_LIGHT ? QColor(242, 222, 222) : QColor(133, 52, 52);
            case SEVERITY_WARN:
                return m_colorTheme == THEME_LIGHT ? QColor(252, 248, 227) : QColor(173, 150, 18);
            case SEVERITY_NOTICE:
                if (m_colorBackground == COLOR_ALL)
                {
                    return m_colorTheme == THEME_LIGHT ? QColor(223, 240, 216) : QColor(75, 132, 51);
                }
                return QVariant();
            case SEVERITY_INFO:
                if (m_colorBackground == COLOR_ALL)
                {
                    return m_colorTheme == THEME_LIGHT ? QColor(217, 237, 247) : QColor(32, 114, 153);
                }
                return QVariant();
            default:
                return QVariant();
            }
        }
    }
    switch (index.column())
    {
    case 0:
        switch (m_timestampPrecision)
        {
        case PRECISION_SECONDS:
            return QLocale::system().toString(message.timestamp.date(), QLocale::NarrowFormat) + " " +
                    QLocale::system().toString(message.timestamp.time(), QLocale::LongFormat);
        case PRECISION_MILLISECONDS:
            return QLocale::system().toString(message.timestamp.date(), QLocale::NarrowFormat) + " " +
                    QLocale::system().toString(message.timestamp.time(), QLocale::LongFormat) + message.timestamp.toString(" zzz");
        default:
            return message.timestamp;
        }
    case 1:
        return message.pid;
    case 2:
        return message.executablePath;
    case 3:
        return message.machineName;
    case 4:
        return message.module;
    case 5:
        return message.channel;
    default:
        if (m_splitByPids)
        {
            auto pidIndex = index.column() - 6;
            if (pidIndex < m_pids.size())
            {
                return m_pids[pidIndex] == message.pid ? message.message : "";
            }
        }
        else
        {
            return message.message;
        }
        return QVariant();
    }
}

QVariant AbstractLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0)
    {
        return QVariant();
    }
    if (orientation == Qt::Vertical && role == Qt::DisplayRole && section == m_messages.size())
    {
        return QVariant();
    }
    else if (orientation == Qt::Vertical && role == Qt::DecorationRole && section < m_messages.size())
    {
        auto& message = *m_messages[section];
        if (message.severity < SEVERITY_COUNT)
        {
            return m_logTypes[message.severity];
        }
    }
    else if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return "Time";
        case 1:
            return "PID";
        case 2:
            return "Executable";
        case 3:
            return "Machine";
        case 4:
            return "Module";
        case 5:
            return "Channel";
        default:
            if (m_splitByPids && !m_pids.empty())
            {
                auto pidIndex = section - 6;
                if (pidIndex < m_pids.size())
                {
                    return QString("%1 Message").arg(m_pids[pidIndex]);
                }
            }
            else
            {
                return "Message";
            }
            break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void AbstractLogModel::addMessage(LogMessage* message)
{
    bool isMultiline = message->message.contains('\n');
    message->isMultilineContinuation = false;
    message->originalMessage = message->message;
    if (m_breakLines && isMultiline)
    {
        auto lines = message->message.split('\n');
        message->message = lines[0];
        m_messages.append(message);
        for (int i = 1; i < lines.size(); ++i)
        {
            LogMessage* newMessage = new LogMessage;
            newMessage->channel = message->channel;
            newMessage->executablePath = message->executablePath;
            newMessage->isMultilineContinuation = true;
            newMessage->machineName = message->machineName;
            newMessage->message = lines[i];
            newMessage->module = message->module;
            newMessage->pid = message->pid;
            newMessage->severity = message->severity;
            newMessage->timestamp = message->timestamp;
            m_messages.append(newMessage);
        }
    }
    else
    {
        m_messages.append(message);
    }
    if (!m_pids.contains(message->pid))
    {
        m_pids.append(message->pid);
        if (m_splitByPids && m_pids.size() > 1)
        {
            beginInsertColumns(QModelIndex(), 6 + m_pids.size() - 1, 6 + m_pids.size() - 1);
            endInsertColumns();
        }
    }
}

void AbstractLogModel::refreshColorBackgroundTheme()
{
    QVector<int> roles;
    roles.append(Qt::BackgroundRole);
    dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), roles);
}

void AbstractLogModel::explodeMessages()
{
    for (int i = 0; i < m_messages.size(); ++i)
    {
        auto message = m_messages[i];
        if (message->isMultilineContinuation || !message->message.contains('\n'))
        {
            continue;
        }
        auto lines = message->message.split('\n');
        message->message = lines[0];
        for (int j = 1; j < lines.size(); ++j)
        {
            LogMessage* newMessage = new LogMessage;
            newMessage->channel = message->channel;
            newMessage->executablePath = message->executablePath;
            newMessage->isMultilineContinuation = true;
            newMessage->machineName = message->machineName;
            newMessage->message = lines[j];
            newMessage->module = message->module;
            newMessage->pid = message->pid;
            newMessage->severity = message->severity;
            newMessage->timestamp = message->timestamp;
            m_messages.insert(i + j, newMessage);
        }
        dataChanged(index(i, 6), index(i, 6));
        beginInsertRows(QModelIndex(), i + 1, i + lines.size() - 1);
        endInsertRows();
    }
}

void AbstractLogModel::collapseMessages()
{
    for (int i = 0; i < m_messages.size(); )
    {
        auto message = m_messages[i];
        if (message->isMultilineContinuation)
        {
            if (i > 0)
            {
                m_messages[i - 1]->message = m_messages[i - 1]->originalMessage;
                dataChanged(index(i - 1, 6), index(i - 1, 6));
            }
            int j;
            for (j = i + 1; j < m_messages.size(); ++j)
            {
                if (!m_messages[j]->isMultilineContinuation)
                {
                    break;
                }
                delete m_messages[j];
            }
            m_messages.remove(i, j - i);
            beginRemoveRows(QModelIndex(), i, j - 1);
            endRemoveRows();
        }
        else
        {
            ++i;
        }
    }

}

void AbstractLogModel::setTimestampPrecision(TimestampPrecision precision)
{
    if (m_timestampPrecision == precision)
    {
        return;
    }
    m_timestampPrecision = precision;
    dataChanged(index(0, 0), index(m_messages.size() - 1, 0));
}
