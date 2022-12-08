#include "logfilter.h"
#include <QColor>
#include <QStandardPaths>
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
    : QSortFilterProxyModel(parent),
      m_severity(0xffffffff),
      m_hasCustomFilter(false),
      m_hasHighlight(false)
{

}

void LogFilter::filterSeverity(LogSeverity severity, bool visible)
{
    auto prevSeverity = m_severity;
    if (visible)
    {
        m_severity |= 1 << severity;
    }
    else
    {
        m_severity &= ~(1 << severity);
    }
    if (prevSeverity != m_severity)
    {
        invalidateFilter();
    }
}

bool LogFilter::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    auto model = static_cast<AbstractLogModel*>(sourceModel());
    if (sourceRow + 1 == model->rowCount())
    {
        return true;
    }
    auto message = model->message(sourceRow);
    if (!message)
    {
        return false;
    }
    if (((1 << message->severity) & m_severity) == 0)
    {
        return false;
    }
    auto filter = filterRegularExpression();
    if (!filter.pattern().isEmpty() && !message->channel.contains(filter) &&
            !message->module.contains(filter) && !message->message.contains(filter))
    {
        return false;
    }
    if (m_hasCustomFilter)
    {
        return m_customFilter.applies(message);
    }
    return true;
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
    invalidateFilter();
}

const Filter* LogFilter::customFilter() const
{
    return m_hasCustomFilter ? &m_customFilter : nullptr;
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
    dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

const HighlightSet* LogFilter::highlight() const
{
    return m_hasHighlight ? &m_highlight : nullptr;
}

QVariant LogFilter::data(const QModelIndex& index, int role) const
{
    if (m_hasHighlight)
    {
        if (role == Qt::ForegroundRole)
        {
            auto model = static_cast<AbstractLogModel*>(sourceModel());
            auto message = model->message(mapToSource(index).row());
            if (message)
            {
                auto color = m_highlight.getForegroundColor(message);
                if (color.isValid())
                {
                    return color;
                }
            }
        }
        else if (role == Qt::BackgroundRole)
        {
            auto model = static_cast<AbstractLogModel*>(sourceModel());
            auto message = model->message(mapToSource(index).row());
            if (message)
            {
                auto color = m_highlight.getBackgroundColor(message);
                if (color.isValid())
                {
                    return color;
                }
            }
        }
    }
    return QSortFilterProxyModel::data(index, role);
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
