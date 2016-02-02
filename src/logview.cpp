#include "logview.h"
#include "logfilter.h"

LogView::LogView(QWidget *parent)
    :QTableView(parent)
{
}

void LogView::setModel(QAbstractItemModel *newModel)
{
    if (model())
    {
        disconnect(model(), nullptr, this, nullptr);
    }
    QTableView::setModel(newModel);
    if (newModel)
    {
        connect(newModel, &QAbstractItemModel::rowsInserted, this, &LogView::autoScroll);
    }
}

AbstractLogModel* LogView::sourceModel() const
{
    auto filter = static_cast<QAbstractProxyModel*>(model());
    return static_cast<AbstractLogModel*>(filter->sourceModel());
}

const LogMessage* LogView::message(int row) const
{
    auto filter = static_cast<QAbstractProxyModel*>(model());
    auto source = static_cast<AbstractLogModel*>(filter->sourceModel());
    return source->message(filter->mapToSource(filter->index(row, 0)).row());
}

void LogView::nextSeverity(LogSeverity type)
{
    auto selection = selectionModel()->currentIndex().row() + 1;
    auto filter = static_cast<QAbstractProxyModel*>(model());
    auto count = filter->rowCount();
    for (int i = 0; i < count; ++i)
    {
        auto msg = message(selection + i);
        if (msg && msg->severity == type)
        {
            selectionModel()->setCurrentIndex(filter->index((selection + i) % count, 0),
                                              QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            return;
        }
    }
}

void LogView::previousSeverity(LogSeverity type)
{
    auto filter = static_cast<QAbstractProxyModel*>(model());
    auto count = filter->rowCount();
    auto selection = selectionModel()->currentIndex().row() + count;
    for (int i = 0; i < count; ++i)
    {
        auto msg = message((selection + count - 1 - i) % count);
        if (msg && msg->severity == type)
        {
            selectionModel()->setCurrentIndex(filter->index((selection + count - 1 - i) % count, 0),
                                              QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            return;
        }
    }
}

void LogView::nextError()
{
    nextSeverity(SEVERITY_ERR);
}

void LogView::previousError()
{
    previousSeverity(SEVERITY_ERR);
}

void LogView::nextWarning()
{
    nextSeverity(SEVERITY_WARN);
}

void LogView::previousWarning()
{
    previousSeverity(SEVERITY_WARN);
}

void LogView::nextNotice()
{
    nextSeverity(SEVERITY_NOTICE);
}

void LogView::previousNotice()
{
    previousSeverity(SEVERITY_NOTICE);
}

void LogView::autoScroll()
{
    if (currentIndex().row() == model()->rowCount() - 1)
    {
        scrollToBottom();
    }
}

void LogView::scrollToBeginning()
{
    selectRow(0);
}

void LogView::scrollToEnd()
{
    selectRow(model()->rowCount() - 1);
}

QString LogView::selectionAsText() const
{
    auto selection = selectionModel()->selectedRows();
    auto source = sourceModel();

    QString text;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto msg = message(selection[i].row());
        if (!msg)
        {
            continue;
        }
        switch (msg->severity)
        {
        case SEVERITY_INFO:
            text += "info";
            break;
        case SEVERITY_NOTICE:
            text += "notice";
            break;
        case SEVERITY_WARN:
            text += "warning";
            break;
        case SEVERITY_ERR:
            text += "error";
            break;
        default:
            break;
        }
        for (int j = 0; j < source->columnCount(); ++j)
        {
            if (isColumnHidden(j))
            {
                continue;
            }
            text += "\t";
            text += model()->data(model()->index(selection[i].row(), j)).toString();
        }
        text += "\n";
    }
    return text;
}

QString LogView::selectionMessagesAsText() const
{
    auto selection = selectionModel()->selectedRows();

    QString text;
    for (int i = 0; i < selection.size(); ++i)
    {
        auto msg = message(selection[i].row());
        if (!msg)
        {
            continue;
        }
        text += model()->data(model()->index(selection[i].row(), LOGFIELD_MESSAGE)).toString();
        text += "\n";
    }
    return text;
}

void LogView::selectNextMatching(QString text)
{
    if (text.isEmpty())
    {
        return;
    }
    auto selection = selectionModel()->currentIndex().row() + 1;
    auto count = model()->rowCount();
    for (int i = 0; i < count; ++i)
    {
        auto msg = message(selection + i);
        if (!msg)
        {
            continue;
        }
        if (msg->message.contains(text, Qt::CaseInsensitive))
        {
            selectionModel()->setCurrentIndex(model()->index((selection + i) % count, 0),
                                              QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            return;
        }
    }
}

void LogView::selectPreviousMatching(QString text)
{
    if (text.isEmpty())
    {
        return;
    }
    auto count = model()->rowCount();
    auto selection = selectionModel()->currentIndex().row() + count;
    for (int i = 0; i < count; ++i)
    {
        auto msg = message((selection + count - 1 - i) % count);
        if (!msg)
        {
            continue;
        }
        if (msg->message.contains(text, Qt::CaseInsensitive))
        {
            selectionModel()->setCurrentIndex(model()->index((selection + count - 1 - i) % count, 0),
                                              QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            return;
        }
    }
}
