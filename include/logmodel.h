#ifndef LOGMODEL_H
#define LOGMODEL_H

#include "abstractlogmodel.h"
#include <QtNetwork/QTcpServer>

class QSettings;

// The single policy that decides what happens to the log buffer when it reaches
// the cap. Exactly one is active at a time, so a combined state is
// unrepresentable. The integer values are persisted to disk and must stay
// stable across versions; new modes append (3+). Unbounded lets the buffer grow
// forever; Server autosaves the whole buffer to a file and then clears it;
// Rolling drops the oldest rows to hold exactly the cap.
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

    // Resolves the retention mode to apply on load. If the "retentionMode" key
    // is present its stored integer maps straight to the enum (0/1/2), so a
    // persisted mode is honored even if the current UI has no control to select
    // it yet. If the key is absent it migrates the legacy "serverMode" bool
    // (true -> Server, false/absent -> Unbounded). An out-of-range stored value
    // falls back to Unbounded.
    static RetentionMode retentionModeFromSettings(QSettings& settings);
protected:
    // Applies the active retention policy to the current buffer. Called after a
    // batch of messages is appended. Server autosaves and clears at the cap;
    // Unbounded and Rolling do not evict here yet.
    void enforceRetentionPolicy();
private:
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
