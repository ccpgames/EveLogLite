#include "logfilter.h"
#include <QColor>
#include <QStandardPaths>
#include <algorithm>
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Filter::Condition::Condition()
    : m_field(LOGFIELD_SEVERITY),
      m_op(Filter::EQUALS),
      m_operand(0)
{
}

bool Filter::Condition::load(QJsonObject json)
{
    auto field = json["field"].toString();
    bool ok;
    m_field = Filter::fieldFromString(json["field"].toString(), &ok);
    if (!ok)
    {
        return false;
    }
    m_op = Filter::operatorFromString(json["operator"].toString(), &ok);
    if (!ok)
    {
        return false;
    }
    m_operand = QJsonValue(json["operand"]).toVariant();
    return true;
}

QJsonObject Filter::Condition::save() const
{
    QJsonObject result;
    result["field"] = Filter::fieldToString(m_field);
    result["operator"] = Filter::operatorToString(m_op);
    result["operand"] = QJsonValue::fromVariant(m_operand);
    return result;
}


bool Filter::Condition::evaluateOperator(Operator op, int operand0, int operand1)
{
    switch (op)
    {
    case EQUALS:
        return operand0 == operand1;
    case NOT_EQUALS:
        return operand0 != operand1;
    case CONTAINS:
        return operand0 == operand1;
    case NOT_CONTAINS:
        return operand0 != operand1;
    case GT:
        return operand0 > operand1;
    case GTE:
        return operand0 >= operand1;
    case LT:
        return operand0 < operand1;
    case LTE:
        return operand0 >= operand1;
    default:
        return false;
    }
}

bool Filter::Condition::evaluateOperator(Operator op, QString operand0, QString operand1)
{
    switch (op)
    {
    case EQUALS:
        return operand0 == operand1;
    case NOT_EQUALS:
        return operand0 != operand1;
    case CONTAINS:
        return operand0.indexOf(operand1) >= 0;
    case NOT_CONTAINS:
        return operand0.indexOf(operand1) < 0;
    case GT:
        return operand0 > operand1;
    case GTE:
        return operand0 >= operand1;
    case LT:
        return operand0 < operand1;
    case LTE:
        return operand0 >= operand1;
    default:
        return false;
    }
}

bool Filter::Condition::applies(const LogMessage *message) const
{
    switch (m_field)
    {
    case LOGFIELD_SEVERITY:
        return evaluateOperator(m_op, message->severity, m_operand.toInt());
    case LOGFIELD_TIMESTAMP:
        return evaluateOperator(m_op, message->timestamp.toString(), m_operand.toString());
    case LOGFIELD_PID:
        return evaluateOperator(m_op, QString::number(message->pid), m_operand.toString());
    case LOGFIELD_EXE_PATH:
        return evaluateOperator(m_op, message->executablePath, m_operand.toString());
    case LOGFIELD_MACHINE:
        return evaluateOperator(m_op, message->machineName, m_operand.toString());
    case LOGFIELD_MODULE:
        return evaluateOperator(m_op, message->module, m_operand.toString());
    case LOGFIELD_CHANNEL:
        return evaluateOperator(m_op, message->channel, m_operand.toString());
    default:
        return evaluateOperator(m_op, message->message, m_operand.toString());
    }
}


Filter::Filter()
    : m_juncture(OR)
{
}

bool Filter::load(QJsonObject object)
{
    bool ok;
    m_juncture = boolOperatorFromString(object["juncture"].toString(), &ok);
    if (!ok)
    {
        return false;
    }
    m_name = object["name"].toString();
    auto conditions = object["conditions"].toArray();
    for (auto ic = conditions.begin(); ic != conditions.end(); ++ic)
    {
        Filter::Condition c;
        if (!c.load(ic->toObject()))
        {
            return false;
        }
        m_conditions.push_back(c);
    }
    return true;
}

bool Filter::load(QString path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly))
    {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
    {
        return false;
    }
    return load(doc.object());
}

QJsonObject Filter::save() const
{
    QJsonObject result;
    result["name"] = m_name;
    result["juncture"] = boolOperatorToString(m_juncture);
    auto conditions = QJsonArray();
    for (auto it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        conditions.append(it->save());
    }
    result["conditions"] = conditions;
    return result;
}

bool Filter::save(QString path) const
{
    QFile f(path);
    if (!f.open(QFile::WriteOnly))
    {
        return false;
    }
    QJsonDocument doc(save());
    f.write(doc.toJson());
    return true;
}

bool Filter::applies(const LogMessage *message, const Conditions& conditions, BoolOperator juncture)
{
    if (juncture == AND)
    {
        for (auto it = conditions.begin(); it != conditions.end(); ++it)
        {
            if (!it->applies(message))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        for (auto it = conditions.begin(); it != conditions.end(); ++it)
        {
            if (it->applies(message))
            {
                return true;
            }
        }
        return false;
    }
}

bool Filter::applies(const LogMessage* message) const
{
    return applies(message, m_conditions, m_juncture);
}

QVector<Filter>& Filter::getFilters()
{
    static QVector<Filter> s_filters;
    static bool s_filtersLoaded = false;
    if (!s_filtersLoaded)
    {
        s_filtersLoaded = true;
        auto dir = settingsDirectory();
        auto names = dir.entryList(QStringList() << "*.filter", QDir::Files);
        for (auto it = names.begin(); it != names.end(); ++it)
        {
            Filter filter;
            if (filter.load(dir.absoluteFilePath(*it)))
            {
                s_filters.append(filter);
            }
        }
    }
    return s_filters;
}

void Filter::saveFilters()
{
    auto& filters = getFilters();
    auto dir = settingsDirectory();
    auto names = dir.entryList(QStringList() << "*.filter", QDir::Files);
    for (auto it = filters.begin(); it != filters.end(); ++it)
    {
        auto name = it->m_name + ".filter";
        auto index = names.indexOf(name);
        if (index >= 0)
        {
            names.removeAt(index);
        }
        it->save(dir.absoluteFilePath(name));
    }
    for (auto it = names.begin(); it != names.end(); ++it)
    {
        dir.remove(*it);
    }
}

QDir Filter::settingsDirectory()
{
    QDir localDataDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    localDataDir.mkpath(QDir::toNativeSeparators(localDataDir.path()));
    return localDataDir;
}

const char** Filter::logFieldNames()
{
    static const char* s_names[] = {
        "severity",
        "timestamp",
        "pid",
        "exepath",
        "machine",
        "module",
        "channel",
        "message"
    };
    return s_names;
}

const char** Filter::operatorNames()
{
    static const char* s_names[] = {
        "equals",
        "not_equals",
        "contains",
        "not_contains",
        "gt",
        "gte",
        "ls",
        "lte"
    };
    return s_names;
}

const char** Filter::boolOperatorNames()
{
    static const char* s_names[] = {
        "and",
        "or",
    };
    return s_names;
}

QString Filter::fieldToString(LogField field)
{
    return logFieldNames()[field + 1];
}

LogField Filter::fieldFromString(QString string, bool* ok)
{
    auto names = logFieldNames();
    for (int i = 0; i <= LOGFIELD_MESSAGE + 1; ++i)
    {
        if (string == names[i])
        {
            if (ok)
            {
                *ok = true;
            }
            return LogField(i - 1);
        }
    }
    if (ok)
    {
        *ok = false;
    }
    return LOGFIELD_MESSAGE;
}

QString Filter::operatorToString(Operator op)
{
    return operatorNames()[op];
}

Filter::Operator Filter::operatorFromString(QString string, bool* ok)
{
    auto names = operatorNames();
    for (int i = 0; i <= LTE; ++i)
    {
        if (string == names[i])
        {
            if (ok)
            {
                *ok = true;
            }
            return Operator(i);
        }
    }
    if (ok)
    {
        *ok = false;
    }
    return EQUALS;
}

QString Filter::boolOperatorToString(BoolOperator op)
{
    return boolOperatorNames()[op];
}

Filter::BoolOperator Filter::boolOperatorFromString(QString string, bool* ok)
{
    auto names = boolOperatorNames();
    for (int i = 0; i <= OR; ++i)
    {
        if (string == names[i])
        {
            if (ok)
            {
                *ok = true;
            }
            return BoolOperator(i);
        }
    }
    if (ok)
    {
        *ok = false;
    }
    return AND;
}



HighlightSet::Highlight::Highlight()
    : m_juncture(Filter::OR)
{
}

bool HighlightSet::Highlight::load(QJsonObject object)
{
    bool ok;
    m_juncture = Filter::boolOperatorFromString(object["juncture"].toString(), &ok);
    if (!ok)
    {
        return false;
    }
    if (object.contains("foreground"))
    {
        auto foreground = object["foreground"].toArray();
        m_foreground = QColor(foreground[0].toInt(), foreground[1].toInt(), foreground[2].toInt());
    }
    else
    {
        m_foreground = QColor();
    }
    if (object.contains("background"))
    {
        auto background = object["background"].toArray();
        m_background = QColor(background[0].toInt(), background[1].toInt(), background[2].toInt());
    }
    else
    {
        m_background = QColor();
    }
    auto conditions = object["conditions"].toArray();
    for (auto it = conditions.begin(); it != conditions.end(); ++it)
    {
        Filter::Condition c;
        if (!c.load(it->toObject()))
        {
            return false;
        }
        m_conditions.append(c);
    }
    return true;
}

QJsonObject HighlightSet::Highlight::save() const
{
    QJsonObject result;
    result["juncture"] = Filter::boolOperatorToString(m_juncture);
    QJsonArray jsonArray;
    if (m_foreground.isValid())
    {
        jsonArray = QJsonArray();
        jsonArray.append(m_foreground.red());
        jsonArray.append(m_foreground.green());
        jsonArray.append(m_foreground.blue());
        result["foreground"] = jsonArray;
    }
    if (m_background.isValid())
    {
        jsonArray = QJsonArray();
        jsonArray.append(m_background.red());
        jsonArray.append(m_background.green());
        jsonArray.append(m_background.blue());
        result["background"] = jsonArray;
    }
    jsonArray = QJsonArray();
    for (auto it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        jsonArray.append(it->save());
    }
    result["conditions"] = jsonArray;
    return result;
}


bool HighlightSet::load(QJsonObject object)
{
    m_name = object["name"].toString();
    auto highlights = object["highlights"].toArray();
    for (auto it = highlights.begin(); it != highlights.end(); ++it)
    {
        Highlight highlight;
        if (!highlight.load(it->toObject()))
        {
            return false;
        }
        m_highlights.append(highlight);
    }
    return true;
}

bool HighlightSet::load(QString path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly))
    {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
    {
        return false;
    }
    return load(doc.object());
}

QJsonObject HighlightSet::save() const
{
    QJsonObject result;
    result["name"] = m_name;
    QJsonArray highlights;
    for (auto it = m_highlights.begin(); it != m_highlights.end(); ++it)
    {
        highlights.append(it->save());
    }
    result["highlights"] = highlights;
    return result;
}

bool HighlightSet::save(QString path) const
{
    QFile f(path);
    if (!f.open(QFile::WriteOnly))
    {
        return false;
    }
    QJsonDocument doc(save());
    f.write(doc.toJson());
    return true;
}

QColor HighlightSet::getForegroundColor(const LogMessage* message) const
{
    for (auto it = m_highlights.begin(); it != m_highlights.end(); ++it)
    {
        if (Filter::applies(message, it->m_conditions, it->m_juncture))
        {
            return QColor(it->m_foreground);
        }
    }
    return QColor();
}

QColor HighlightSet::getBackgroundColor(const LogMessage* message) const
{
    for (auto it = m_highlights.begin(); it != m_highlights.end(); ++it)
    {
        if (Filter::applies(message, it->m_conditions, it->m_juncture))
        {
            return QColor(it->m_background);
        }
    }
    return QColor();
}

QVector<HighlightSet>& HighlightSet::getSets()
{
    static QVector<HighlightSet> s_sets;
    static bool s_setsLoaded = false;
    if (!s_setsLoaded)
    {
        s_setsLoaded = true;
        auto dir = Filter::settingsDirectory();
        auto names = dir.entryList(QStringList() << "*.highlight", QDir::Files);
        for (auto it = names.begin(); it != names.end(); ++it)
        {
            HighlightSet set;
            if (set.load(dir.absoluteFilePath(*it)))
            {
                s_sets.append(set);
            }
        }
    }
    return s_sets;
}

void HighlightSet::saveSets()
{
    auto& sets = getSets();
    auto dir = Filter::settingsDirectory();
    auto names = dir.entryList(QStringList() << "*.highlight", QDir::Files);
    for (auto it = sets.begin(); it != sets.end(); ++it)
    {
        auto name = it->m_name + ".highlight";
        auto index = names.indexOf(name);
        if (index >= 0)
        {
            names.removeAt(index);
        }
        it->save(dir.absoluteFilePath(name));
    }
    for (auto it = names.begin(); it != names.end(); ++it)
    {
        dir.remove(*it);
    }
}




LogFilter::LogFilter(QObject* parent)
    : QAbstractProxyModel(parent),
      m_severity(0xffffffff),
      m_hasCustomFilter(false),
      m_hasHighlight(false),
      m_pendingRemoveLo(0),
      m_pendingRemoveCount(0)
{

}

AbstractLogModel* LogFilter::logModel() const
{
    return static_cast<AbstractLogModel*>(sourceModel());
}

int LogFilter::messageCount() const
{
    auto model = sourceModel();
    return model ? model->rowCount() - 1 : 0;
}

bool LogFilter::accepts(const LogMessage* message) const
{
    if (((1u << message->severity) & m_severity) == 0)
    {
        return false;
    }
    if (!m_filterText.isEmpty() &&
            !message->channel.contains(m_filterText, Qt::CaseInsensitive) &&
            !message->module.contains(m_filterText, Qt::CaseInsensitive) &&
            !message->message.contains(m_filterText, Qt::CaseInsensitive))
    {
        return false;
    }
    if (m_hasCustomFilter)
    {
        return m_customFilter.applies(message);
    }
    return true;
}

void LogFilter::rebuildMapping()
{
    m_proxyToSource.clear();
    auto model = logModel();
    if (!model)
    {
        return;
    }
    const int count = messageCount();
    m_proxyToSource.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        auto message = model->message(i);
        if (message && accepts(message))
        {
            m_proxyToSource.append(i);
        }
    }
}

void LogFilter::refilter()
{
    if (!sourceModel())
    {
        return;
    }

    emit layoutAboutToBeChanged();

    // Remember what each persistent index points at (in source space) so
    // selection and the current row survive the change to the visible set.
    const QModelIndexList oldIndexes = persistentIndexList();
    QVector<int> sourceRows;
    QVector<int> columns;
    sourceRows.reserve(oldIndexes.size());
    columns.reserve(oldIndexes.size());
    for (const QModelIndex& idx : oldIndexes)
    {
        sourceRows.append(mapToSource(idx).row());
        columns.append(idx.column());
    }

    rebuildMapping();

    QModelIndexList newIndexes;
    newIndexes.reserve(oldIndexes.size());
    for (int i = 0; i < oldIndexes.size(); ++i)
    {
        newIndexes.append(mapFromSource(sourceModel()->index(sourceRows[i], columns[i])));
    }
    changePersistentIndexList(oldIndexes, newIndexes);

    emit layoutChanged();
}

void LogFilter::setSourceModel(QAbstractItemModel* newSourceModel)
{
    beginResetModel();
    if (sourceModel())
    {
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &LogFilter::sourceRowsInserted);
        disconnect(sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &LogFilter::sourceRowsAboutToBeRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &LogFilter::sourceRowsRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, &LogFilter::sourceDataChanged);
        disconnect(sourceModel(), &QAbstractItemModel::columnsAboutToBeInserted, this, &LogFilter::sourceColumnsAboutToBeInserted);
        disconnect(sourceModel(), &QAbstractItemModel::columnsInserted, this, &LogFilter::sourceColumnsInserted);
        disconnect(sourceModel(), &QAbstractItemModel::columnsAboutToBeRemoved, this, &LogFilter::sourceColumnsAboutToBeRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::columnsRemoved, this, &LogFilter::sourceColumnsRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &LogFilter::sourceAboutToBeReset);
        disconnect(sourceModel(), &QAbstractItemModel::modelReset, this, &LogFilter::sourceReset);
    }
    QAbstractProxyModel::setSourceModel(newSourceModel);
    if (newSourceModel)
    {
        connect(newSourceModel, &QAbstractItemModel::rowsInserted, this, &LogFilter::sourceRowsInserted);
        connect(newSourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &LogFilter::sourceRowsAboutToBeRemoved);
        connect(newSourceModel, &QAbstractItemModel::rowsRemoved, this, &LogFilter::sourceRowsRemoved);
        connect(newSourceModel, &QAbstractItemModel::dataChanged, this, &LogFilter::sourceDataChanged);
        connect(newSourceModel, &QAbstractItemModel::columnsAboutToBeInserted, this, &LogFilter::sourceColumnsAboutToBeInserted);
        connect(newSourceModel, &QAbstractItemModel::columnsInserted, this, &LogFilter::sourceColumnsInserted);
        connect(newSourceModel, &QAbstractItemModel::columnsAboutToBeRemoved, this, &LogFilter::sourceColumnsAboutToBeRemoved);
        connect(newSourceModel, &QAbstractItemModel::columnsRemoved, this, &LogFilter::sourceColumnsRemoved);
        connect(newSourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &LogFilter::sourceAboutToBeReset);
        connect(newSourceModel, &QAbstractItemModel::modelReset, this, &LogFilter::sourceReset);
    }
    rebuildMapping();
    endResetModel();
}

QModelIndex LogFilter::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!proxyIndex.isValid() || !sourceModel())
    {
        return QModelIndex();
    }
    const int row = proxyIndex.row();
    int sourceRow;
    if (row < m_proxyToSource.size())
    {
        sourceRow = m_proxyToSource[row];
    }
    else if (row == m_proxyToSource.size())
    {
        // trailing placeholder row, always present
        sourceRow = messageCount();
    }
    else
    {
        return QModelIndex();
    }
    return sourceModel()->index(sourceRow, proxyIndex.column());
}

QModelIndex LogFilter::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid() || !sourceModel())
    {
        return QModelIndex();
    }
    const int sourceRow = sourceIndex.row();
    int proxyRow;
    if (sourceRow == messageCount())
    {
        // trailing placeholder row
        proxyRow = m_proxyToSource.size();
    }
    else
    {
        auto it = std::lower_bound(m_proxyToSource.begin(), m_proxyToSource.end(), sourceRow);
        if (it == m_proxyToSource.end() || *it != sourceRow)
        {
            return QModelIndex(); // filtered out
        }
        proxyRow = int(it - m_proxyToSource.begin());
    }
    return index(proxyRow, sourceIndex.column());
}

QModelIndex LogFilter::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid() || row < 0 || column < 0 ||
            row >= rowCount() || column >= columnCount())
    {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex LogFilter::parent(const QModelIndex&) const
{
    return QModelIndex();
}

QModelIndex LogFilter::sibling(int row, int column, const QModelIndex& idx) const
{
    // Flat model: siblings share the (invalid) root parent. Resolve in proxy
    // space rather than mapping through the source.
    return index(row, column, idx.parent());
}

int LogFilter::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !sourceModel())
    {
        return 0;
    }
    // accepted message rows plus the trailing placeholder row
    return m_proxyToSource.size() + 1;
}

int LogFilter::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !sourceModel())
    {
        return 0;
    }
    return sourceModel()->columnCount();
}

void LogFilter::sourceRowsInserted(const QModelIndex& parent, int first, int last)
{
    if (parent.isValid())
    {
        return;
    }
    const int delta = last - first + 1;
    // Shift already-mapped rows that sit at or after the insertion point.
    auto splitIt = std::lower_bound(m_proxyToSource.begin(), m_proxyToSource.end(), first);
    const int split = int(splitIt - m_proxyToSource.begin());
    for (int i = split; i < m_proxyToSource.size(); ++i)
    {
        m_proxyToSource[i] += delta;
    }
    // Test only the newly inserted source rows (never the placeholder, which
    // always stays last), gathering the accepted ones in ascending order.
    QVector<int> accepted;
    auto model = logModel();
    for (int sourceRow = first; sourceRow <= last; ++sourceRow)
    {
        auto message = model->message(sourceRow);
        if (message && accepts(message))
        {
            accepted.append(sourceRow);
        }
    }
    if (accepted.isEmpty())
    {
        return;
    }
    beginInsertRows(QModelIndex(), split, split + accepted.size() - 1);
    for (int i = 0; i < accepted.size(); ++i)
    {
        m_proxyToSource.insert(split + i, accepted[i]);
    }
    endInsertRows();
}

void LogFilter::sourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    m_pendingRemoveCount = 0;
    if (parent.isValid())
    {
        return;
    }
    // Because m_proxyToSource is sorted, the accepted rows falling inside the
    // removed source range [first, last] form a single contiguous proxy run.
    auto loIt = std::lower_bound(m_proxyToSource.begin(), m_proxyToSource.end(), first);
    auto hiIt = std::lower_bound(m_proxyToSource.begin(), m_proxyToSource.end(), last + 1);
    m_pendingRemoveLo = int(loIt - m_proxyToSource.begin());
    m_pendingRemoveCount = int(hiIt - loIt);
    if (m_pendingRemoveCount > 0)
    {
        beginRemoveRows(QModelIndex(), m_pendingRemoveLo, m_pendingRemoveLo + m_pendingRemoveCount - 1);
    }
}

void LogFilter::sourceRowsRemoved(const QModelIndex& parent, int first, int last)
{
    if (parent.isValid())
    {
        return;
    }
    const int delta = last - first + 1;
    if (m_pendingRemoveCount > 0)
    {
        m_proxyToSource.remove(m_pendingRemoveLo, m_pendingRemoveCount);
    }
    // Everything now at or after the removal point referenced source rows beyond
    // the removed range, so slide their source indexes down.
    for (int i = m_pendingRemoveLo; i < m_proxyToSource.size(); ++i)
    {
        m_proxyToSource[i] -= delta;
    }
    if (m_pendingRemoveCount > 0)
    {
        endRemoveRows();
        m_pendingRemoveCount = 0;
    }
}

void LogFilter::sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (topLeft.parent().isValid() || bottomRight.parent().isValid() || !sourceModel())
    {
        return;
    }
    const int sourceFirst = topLeft.row();
    const int sourceLast = bottomRight.row();
    const int placeholder = messageCount();

    auto loIt = std::lower_bound(m_proxyToSource.begin(), m_proxyToSource.end(), sourceFirst);
    auto hiIt = std::upper_bound(m_proxyToSource.begin(), m_proxyToSource.end(), sourceLast);
    int proxyFirst = int(loIt - m_proxyToSource.begin());
    int proxyLast = int(hiIt - m_proxyToSource.begin()) - 1;

    if (sourceFirst <= placeholder && placeholder <= sourceLast)
    {
        // the changed range also covers the trailing placeholder row
        if (proxyLast < proxyFirst)
        {
            proxyFirst = m_proxyToSource.size();
        }
        proxyLast = m_proxyToSource.size();
    }

    if (proxyLast < proxyFirst)
    {
        return;
    }
    emit dataChanged(index(proxyFirst, topLeft.column()), index(proxyLast, bottomRight.column()), roles);
}

void LogFilter::sourceColumnsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    if (!parent.isValid())
    {
        beginInsertColumns(QModelIndex(), first, last);
    }
}

void LogFilter::sourceColumnsInserted(const QModelIndex& parent, int, int)
{
    if (!parent.isValid())
    {
        endInsertColumns();
    }
}

void LogFilter::sourceColumnsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    if (!parent.isValid())
    {
        beginRemoveColumns(QModelIndex(), first, last);
    }
}

void LogFilter::sourceColumnsRemoved(const QModelIndex& parent, int, int)
{
    if (!parent.isValid())
    {
        endRemoveColumns();
    }
}

void LogFilter::sourceAboutToBeReset()
{
    beginResetModel();
}

void LogFilter::sourceReset()
{
    rebuildMapping();
    endResetModel();
}

void LogFilter::filterSeverity(LogSeverity severity, bool visible)
{
    auto prevSeverity = m_severity;
    if (visible)
    {
        m_severity |= 1u << severity;
    }
    else
    {
        m_severity &= ~(1u << severity);
    }
    if (prevSeverity != m_severity)
    {
        refilter();
    }
}

void LogFilter::setCustomFilter(const Filter* filter)
{
    if (filter)
    {
        m_customFilter = *filter;
        m_hasCustomFilter = true;
    }
    else
    {
        m_hasCustomFilter = false;
    }
    refilter();
}

const Filter* LogFilter::customFilter() const
{
    return m_hasCustomFilter ? &m_customFilter : nullptr;
}

void LogFilter::setFilterFixedString(const QString& text)
{
    if (m_filterText == text)
    {
        return;
    }
    m_filterText = text;
    refilter();
}

void LogFilter::setHighlight(const HighlightSet* highlight)
{
    if (highlight)
    {
        m_highlight = *highlight;
        m_hasHighlight = true;
    }
    else
    {
        m_hasHighlight = false;
    }
    if (rowCount() > 0)
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
}

const HighlightSet* LogFilter::highlight() const
{
    return m_hasHighlight ? &m_highlight : nullptr;
}

QVariant LogFilter::data(const QModelIndex& index, int role) const
{
    if (m_hasHighlight && (role == Qt::ForegroundRole || role == Qt::BackgroundRole))
    {
        auto model = logModel();
        if (model)
        {
            auto message = model->message(mapToSource(index).row());
            if (message)
            {
                auto color = role == Qt::ForegroundRole
                        ? m_highlight.getForegroundColor(message)
                        : m_highlight.getBackgroundColor(message);
                if (color.isValid())
                {
                    return color;
                }
            }
        }
    }
    return QAbstractProxyModel::data(index, role);
}

void LogFilter::showErrors(bool show)
{
    filterSeverity(SEVERITY_ERR, show);
}

void LogFilter::showWarnings(bool show)
{
    filterSeverity(SEVERITY_WARN, show);
}

void LogFilter::showNotices(bool show)
{
    filterSeverity(SEVERITY_NOTICE, show);
}

void LogFilter::showInfos(bool show)
{
    filterSeverity(SEVERITY_INFO, show);
}
