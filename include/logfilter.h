#ifndef LOGFILTER_H
#define LOGFILTER_H

#include <QAbstractProxyModel>
#include <QVector>
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


// A purpose-built flat filter proxy for the log models.
//
// QSortFilterProxyModel re-tested every source row and rebuilt its whole mapping
// on each invalidateFilter(), which froze the UI for seconds on large logs when
// toggling the severity buttons. This proxy instead keeps a sorted list of the
// accepted source rows (m_proxyToSource) and, on a filter change, rebuilds that
// list in a single tight loop and emits layoutChanged() (preserving selection via
// persistent indexes) rather than re-testing through the generic proxy machinery.
// Streaming inserts/removals from the source stay incremental and cheap.
class LogFilter : public QAbstractProxyModel
{
    Q_OBJECT

public:
    LogFilter(QObject* parent = nullptr);

    void filterSeverity(LogSeverity severity, bool visible);
    void setCustomFilter(const Filter* filter);
    const Filter* customFilter() const;
    void setHighlight(const HighlightSet* highlight);
    const HighlightSet* highlight() const;

    // Case-insensitive substring filter over channel/module/message.
    void setFilterFixedString(const QString& text);

    void setSourceModel(QAbstractItemModel* sourceModel) override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QModelIndex sibling(int row, int column, const QModelIndex& idx) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public slots:
    void showErrors(bool show);
    void showWarnings(bool show);
    void showNotices(bool show);
    void showInfos(bool show);

private slots:
    void sourceRowsInserted(const QModelIndex& parent, int first, int last);
    void sourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void sourceRowsRemoved(const QModelIndex& parent, int first, int last);
    void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void sourceColumnsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void sourceColumnsInserted(const QModelIndex& parent, int first, int last);
    void sourceColumnsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void sourceColumnsRemoved(const QModelIndex& parent, int first, int last);
    void sourceAboutToBeReset();
    void sourceReset();

private:
    AbstractLogModel* logModel() const;
    // Number of real message rows; the source always exposes one extra trailing
    // placeholder row after them which is never filtered out.
    int messageCount() const;
    bool accepts(const LogMessage* message) const;
    void rebuildMapping();
    void refilter();

    QVector<int> m_proxyToSource;
    uint32_t m_severity;
    Filter m_customFilter;
    HighlightSet m_highlight;
    QString m_filterText;
    bool m_hasCustomFilter;
    bool m_hasHighlight;
    int m_pendingRemoveLo;
    int m_pendingRemoveCount;
};

#endif // LOGFILTER_H
