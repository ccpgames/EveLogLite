#include "logmonitorfilemodel.h"
#include <QtSql>
#include <QDebug>
#include <QMessageBox>

LogMonitorFileModel::LogMonitorFileModel(const QString &dbPath, QObject *parent)
    :AbstractLogModel(parent)
{
    m_statistics.error = 0;
    m_statistics.warning = 0;
    m_statistics.notice = 0;
    m_statistics.info = 0;
    m_statistics.clients = 0;

    QFile f(dbPath);
    if (!f.exists())
    {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("File not found");
        msg.setInformativeText(QString("Could not find file %1").arg(dbPath));
        msg.exec();
        return;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    auto success = db.open();
    if (!success)
    {
        auto e = db.lastError();

        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Open failed");
        msg.setInformativeText(QString("Failed to open the file %1.\nError message: %2").arg(dbPath, e.text()));
        msg.exec();
        return;
    }
    QSqlQuery query("SELECT m.time, m.pid, m.level, h.name, c.facility, c.object, m.message, p.process"
                    " FROM messages AS m INNER JOIN hosts AS h ON m.host=h.id INNER JOIN channels AS c ON m.channel=c.id INNER JOIN processes AS p ON m.pid=p.id",
                    db);
    query.setForwardOnly(true);
    success = query.exec();
    if (!success)
    {
        auto e = db.lastError();

        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Open failed");
        msg.setInformativeText(QString("Failed to open the file %1.\nError message: %2").arg(dbPath, e.text()));
        msg.exec();
        return;
    }
    while (query.next())
    {
        auto message = new LogMessage;
        message->timestamp.setMSecsSinceEpoch(qint64(query.value(0).toDouble() * 1000));
        message->pid = query.value(1).toULongLong();
        message->severity = LogSeverity(query.value(2).toInt());
        message->machineName = query.value(3).toString();
        message->module = query.value(4).toString();
        message->channel = query.value(5).toString();
        message->message = query.value(6).toString();
        message->executablePath = query.value(7).toString();
        switch (message->severity)
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
        addMessage(message);
    }
    if (m_messages.size())
    {
        beginInsertRows(QModelIndex(), 0, m_messages.size() - 1);
        endInsertRows();
    }
    db.close();
}

const LogMonitorFileModel::Statistics &LogMonitorFileModel::statistics() const
{
    return m_statistics;
}

bool LogMonitorFileModel::isListening() const
{
    return false;
}

void LogMonitorFileModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_messages.size() - 1);
    for (int i = 0; i < m_messages.size(); ++i)
    {
        delete m_messages[i];
    }
    m_messages.clear();
    endRemoveRows();

    m_statistics.error = 0;
    m_statistics.warning = 0;
    m_statistics.notice = 0;
    m_statistics.info = 0;
}

bool LogMonitorFileModel::saveModel(AbstractLogModel *model, const QString& fileName)
{
    QTemporaryFile tempFile;
    tempFile.open();

    {
        auto db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(tempFile.fileName());
        auto success = db.open();
        if (!success)
        {
            return false;
        }

        auto check = [&](bool result)
        {
            if (!result)
            {
                auto e = db.lastError();
                qDebug() << "Query failed: " << e.text() << e.nativeErrorCode();
            }
        };

        check(QSqlQuery(db).exec("CREATE TABLE lsw(version INT)"));
        check(QSqlQuery(db).exec("CREATE TABLE messages(time REAL, host INT, pid INT, level INT, channel INT, message TEXT)"));
        check(QSqlQuery(db).exec("CREATE TABLE hosts(id INT,name TEXT)"));
        check(QSqlQuery(db).exec("CREATE TABLE processes(id INT, module TEXT, process TEXT, host INT)"));
        check(QSqlQuery(db).exec("CREATE TABLE channels(id INT,facility TEXT,object TEXT)"));
        check(QSqlQuery(db).exec("CREATE VIEW log as "
                  "select m.rowid, '' as timestamp, m.time, h.name as host, m.pid, m.level, m.level as type, p.module, c.facility || '-' || c.object as channel, m.message, p.process "
                      "from messages as m, hosts as h, processes as p, channels as c "
                      "where h.id = m.host and p.id = m.pid and c.id = m.channel"));



        check(QSqlQuery(db).exec("INSERT INTO lsw VALUES (3)"));

        int count = model->rowCount();
        QMap<QString, int> hosts;
        QMap<quint64, QString> processes;
        QMap<QPair<QString, QString>, int> channels;

        for (int i = 0; i < count; ++i)
        {
            auto message = model->message(i);
            if (!message || message->isMultilineContinuation)
            {
                continue;
            }
            if (hosts.find(message->machineName) == hosts.end())
            {
                hosts[message->machineName] = hosts.size() + 1;
            }
            processes[message->pid] = message->executablePath;
            if (channels.find(QPair<QString, QString>(message->module, message->channel)) == channels.end())
            {
                channels[QPair<QString, QString>(message->module, message->channel)] = channels.size() + 1;
            }
        }

        check(QSqlQuery(db).exec("BEGIN TRANSACTION"));
        {
            QSqlQuery q(db);
            q.prepare("INSERT INTO hosts VALUES (?, ?)");

            QVariantList ids;
            QVariantList names;
            for (auto it = hosts.begin(); it != hosts.end(); ++it)
            {
                ids << it.value();
                names << it.key();
            }
            q.addBindValue(ids);
            q.addBindValue(names);
            check(q.execBatch());
        }
        {
            QSqlQuery q(db);
            q.prepare("INSERT INTO processes VALUES (?, ?, ?, ?)");

            QVariantList ids;
            QVariantList modules;
            QVariantList process;
            QVariantList hosts;
            for (auto it = processes.begin(); it != processes.end(); ++it)
            {
                ids << it.key();
                modules << "";
                process << it.value();
                hosts << 0;
            }
            q.addBindValue(ids);
            q.addBindValue(modules);
            q.addBindValue(process);
            q.addBindValue(hosts);
            check(q.execBatch());
        }
        {
            QSqlQuery q(db);
            q.prepare("INSERT INTO channels VALUES (?, ?, ?)");

            QVariantList ids;
            QVariantList facilities;
            QVariantList objects;
            for (auto it = channels.begin(); it != channels.end(); ++it)
            {
                ids << it.value();
                facilities << it.key().first;
                objects << it.key().second;
            }
            q.addBindValue(ids);
            q.addBindValue(facilities);
            q.addBindValue(objects);
            check(q.execBatch());
        }
        {
            QSqlQuery q(db);
            q.prepare("INSERT INTO messages VALUES (?, ?, ?, ?, ?, ?)");

            QVariantList time;
            QVariantList host;
            QVariantList pid;
            QVariantList level;
            QVariantList channel;
            QVariantList message;
            for (int i = 0; i < count; ++i)
            {
                auto msg = model->message(i);
                if (!msg || msg->isMultilineContinuation)
                {
                    continue;
                }
                time << double(msg->timestamp.toMSecsSinceEpoch()) / 1000;
                host << hosts[msg->machineName];
                pid << msg->pid;
                level << msg->severity;
                channel << channels[QPair<QString, QString>(msg->module, msg->channel)];
                message << msg->originalMessage;
            }
            q.addBindValue(time);
            q.addBindValue(host);
            q.addBindValue(pid);
            q.addBindValue(level);
            q.addBindValue(channel);
            q.addBindValue(message);
            check(q.execBatch());
        }
        check(QSqlQuery(db).exec("END TRANSACTION"));
        db.close();
    }
    QSqlDatabase::removeDatabase("QSQLITE");

    return tempFile.copy(fileName);
}
