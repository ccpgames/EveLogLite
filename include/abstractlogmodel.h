#ifndef ABSTRACTLOGMODEL
#define ABSTRACTLOGMODEL

#include <QAbstractTableModel>
#include <QDateTime>
#include <QIODevice>
#include <QPixmap>
#include <cstdint>


enum LogSeverity
{
    SEVERITY_INFO,
    SEVERITY_NOTICE,
    SEVERITY_WARN,
    SEVERITY_ERR,

    SEVERITY_COUNT,
};

enum LogField
{
    LOGFIELD_SEVERITY = -1,
    LOGFIELD_TIMESTAMP = 0,
    LOGFIELD_PID,
    LOGFIELD_EXE_PATH,
    LOGFIELD_MACHINE,
    LOGFIELD_MODULE,
    LOGFIELD_CHANNEL,
    LOGFIELD_MESSAGE,
};

struct LogMessage
{
    QDateTime timestamp;
    quint64 pid;
    LogSeverity severity;
    QString machineName;
    QString executablePath;
    QString module;
    QString channel;
    QString message;

    QString originalMessage;
    bool isMultilineContinuation;
};

enum LogColorBackground
{
    COLOR_NONE,
    COLOR_ERRORS_WARNINGS,
    COLOR_ALL,
};

enum LogColorTheme
{
    THEME_LIGHT,
    THEME_DARK,
};

enum TimestampPrecision
{
    PRECISION_MINUTES,
    PRECISION_SECONDS,
    PRECISION_MILLISECONDS,
};


class AbstractLogModel: public QAbstractTableModel
{
public:
    AbstractLogModel(QObject* parent = nullptr);

    struct Statistics
    {
        int error;
        int warning;
        int notice;
        int info;
        int clients;
    };

    virtual const Statistics &statistics() const = 0;
    virtual bool isListening() const = 0;

    const LogMessage *message(int index) const;

    void setBreakLines(bool breakLines);
    void setColorBackground(LogColorBackground colorBackground);
    void setColorTheme(LogColorTheme colorTheme);

    void setSplitByPid(bool split);
    bool splitByPid() const;
    void setTimestampPrecision(TimestampPrecision precision);

    void saveAsText(QIODevice* output);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
protected:
    void addMessage(LogMessage*);
    QVector<LogMessage*> m_messages;
    bool m_breakLines;
private:
    void refreshColorBackgroundTheme();
    void explodeMessages();
    void collapseMessages();

    QVector<QPixmap> m_logTypes;
    QList<uint64_t> m_pids;
    bool m_splitByPids;
    TimestampPrecision m_timestampPrecision;
    LogColorBackground m_colorBackground;
    LogColorTheme m_colorTheme;
public slots:
    virtual void clear() = 0;
};

#endif // ABSTRACTLOGMODEL

