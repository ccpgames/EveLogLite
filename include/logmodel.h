#ifndef LOGMODEL_H
#define LOGMODEL_H

#include "abstractlogmodel.h"
#include <QtNetwork/QTcpServer>

class QSettings;

// The integer values are persisted to disk and must stay stable across
// versions; new modes append (3+).
enum class RetentionMode
{
    Unbounded = 0,
    Server = 1,
    Rolling = 2,
};

class LogModel : public AbstractLogModel
{
    Q_OBJECT

public:
    LogModel(QObject* parent = nullptr);
    ~LogModel();

    class Client
    {
    public:
        Client();
        Client(QTcpSocket* socket);

        uint64_t pid() const;
        QString path() const;
        QString machine() const;

        QTcpSocket* socket() const;

        bool operator==(const Client& other) const;
    private:
        QTcpSocket* m_socket;
    };


    typedef QSet<Client> Clients;

    const Clients& clients() const;
    bool clientFromMessage(const LogMessage& message, Client& client);
    void disconnect(const Client& client);

    const Statistics &statistics() const;
    bool isListening() const;

    void setRetentionMode(RetentionMode mode);
    RetentionMode retentionMode() const;
    void setMaxMessages(int maxMessages);
    int maxMessages() const;
    void setAutoSaveDirectory(const QString& autoSaveDirectory);
    QString autoSaveDirectory() const;
    int getRunningCount(LogSeverity severity);

    // Migrates the legacy "serverMode" bool when the "retentionMode" key is
    // absent; an out-of-range stored value falls back to Unbounded.
    static RetentionMode retentionModeFromSettings(QSettings& settings);
protected:
    void recordMessage(LogMessage* message);
    void enforceRetentionPolicy();
private:
    void evictOldest(int count);
    bool autoSave();

    class RunningCount
    {
    public:
        RunningCount();
        void add();
        void update();
        int get() const;
    private:
        QVector<int> m_bins;
        int m_bin;
        int m_count;
    };

    Clients m_clients;

    QTcpServer m_server;
    LogMessage *m_nextMessage;
    QByteArray m_receivedText;
    Statistics m_statistics;
    int m_maxMessages;
    QString m_autoSaveDirectory;
    bool m_listening;
    RetentionMode m_retentionMode;
    RunningCount m_runningCounts[SEVERITY_COUNT];
private slots:
    void acceptConnection();
    void socketDisconnected();
    void readMessages();
    void updateRuningCounts();
public slots:
    void clear();
    void disconnectAll();
signals:
    void clientConnected();
    void clientDisconnected();
};

uint qHash(const LogModel::Client& client);

#endif // LOGMODEL_H
