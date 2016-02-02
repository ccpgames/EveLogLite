#ifndef QLOGLITELOGGER
#define QLOGLITELOGGER

#include <QObject>
#include <QtNetwork/QTcpSocket>

class QLogLiteLogger: public QObject
{
public:
    enum LogSeverity
    {
        SEVERITY_INFO,
        SEVERITY_NOTICE,
        SEVERITY_WARN,
        SEVERITY_ERR,

        SEVERITY_COUNT,
    };

    QLogLiteLogger();

    void connectToHost();
    void connectToHost(qint64 pid, QString machineName, QString executablePath);
    void connectToHost(QString server, qint64 pid, QString machineName, QString executablePath);
    void disconnnect();

    bool isConnected() const;

    void setDefaultModule(QString module);
    QString defaultModule() const;
    void setDefaultChannel(QString channel);
    QString defaultChannel() const;

    void log(LogSeverity severity, QDateTime timestamp, QString module, QString channel, QString message);
    void log(LogSeverity severity, QString module, QString channel, QString message);

    void info(QString message);
    void notice(QString message);
    void warn(QString message);
    void error(QString message);

    void flush();
private:

    struct ConnectionMessage
    {
        static const int MESSAGE_MAX_PATH = 260;

        quint32 version;
        qint64 pid;
        char machineName[32];
        char executablePath[MESSAGE_MAX_PATH];
    };

    struct TextMessage
    {
        static const int TEXT_SIZE = 256;

        quint64 timestamp;
        quint32 severity;
        char module[32];
        char channel[32];
        char message[TEXT_SIZE];
    };

    struct RawLogMessage
    {
        quint32 type;
        union
        {
            ConnectionMessage connection;
            TextMessage text;
        };
    };


    QTcpSocket* m_socket;
    qint64 m_pid;
    QString m_machineName;
    QString m_executablePath;
    QString m_module;
    QString m_channel;
    QVector<RawLogMessage> m_pendingMessages;

    enum State
    {
        Disconnected,
        Connecting,
        Connected,
    };

    State m_state;
private slots:
    void connected();
};



#endif // QLOGLITELOGGER

