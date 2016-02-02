#ifndef LOGFILTER_H
#define LOGFILTER_H

#include <QSortFilterProxyModel>
#include <QColor>
#include "abstractlogmodel.h"

class QDir;

class Filter
{
public:
    enum Operator
    {
        EQUALS,
        NOT_EQUALS,
        CONTAINS,
        NOT_CONTAINS,
        GT,
        GTE,
        LT,
        LTE,
    };

    enum BoolOperator
    {
        AND,
        OR,
    };

    struct Condition
    {
        Condition();
        bool load(QJsonObject json);
        QJsonObject save() const;
        bool applies(const LogMessage *message) const;

        static bool evaluateOperator(Operator op, int operand0, int operand1);
        static bool evaluateOperator(Operator op, QString operand0, QString operand1);

        LogField m_field;
        Operator m_op;
        QVariant m_operand;
    };

    typedef QVector<Condition> Conditions;


    Filter();
    bool load(QJsonObject object);
    bool load(QString path);
    QJsonObject save() const;
    bool save(QString path) const;

    static bool applies(const LogMessage *message, const Conditions& conditions, BoolOperator juncture);
    bool applies(const LogMessage* message) const;

    static QVector<Filter>& getFilters();
    static void saveFilters();

    static QString fieldToString(LogField op);
    static LogField fieldFromString(QString string, bool* ok = nullptr);

    static QString operatorToString(Operator op);
    static Operator operatorFromString(QString string, bool* ok = nullptr);

    static QString boolOperatorToString(BoolOperator op);
    static BoolOperator boolOperatorFromString(QString string, bool* ok = nullptr);

    static QDir settingsDirectory();

    QString m_name;
    BoolOperator m_juncture;
    Conditions m_conditions;
private:
    static const char** logFieldNames();
    static const char** operatorNames();
    static const char** boolOperatorNames();
};

class HighlightSet
{
public:

    struct Highlight
    {
        Highlight();
        bool load(QJsonObject object);
        QJsonObject save() const;

        QColor m_foreground;
        QColor m_background;
        Filter::BoolOperator m_juncture;
        Filter::Conditions m_conditions;
    };

    typedef QVector<Highlight> Highlights;

    bool load(QJsonObject object);
    bool load(QString path);
    QJsonObject save() const;
    bool save(QString path) const;

    QColor getForegroundColor(const LogMessage* message) const;
    QColor getBackgroundColor(const LogMessage* message) const;

    static QVector<HighlightSet>& getSets();
    static void saveSets();

    QString m_name;
    Highlights m_highlights;
};


class LogFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    LogFilter(QObject* parent = nullptr);

    void filterSeverity(LogSeverity severity, bool visible);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    void setCustomFilter(const Filter* filter);
    const Filter* customFilter() const;
    void setHighlight(const HighlightSet* highlight);
    const HighlightSet* highlight() const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
public slots:
    void showErrors(bool show);
    void showWarnings(bool show);
    void showNotices(bool show);
    void showInfos(bool show);
private:
    uint32_t m_severity;
    Filter m_customFilter;
    HighlightSet m_highlight;
    bool m_hasCustomFilter;
    bool m_hasHighlight;
};

#endif // LOGFILTER_H
