#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTableView>
#include <QTimer>
#include "abstractlogmodel.h"

class LogView : public QTableView
{
    Q_OBJECT

public:
    LogView(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel *model);

    AbstractLogModel* sourceModel() const;

    const LogMessage* message(int row) const;
    QString selectionAsText() const;
    QString selectionMessagesAsText() const;

signals:

public slots:
    void nextError();
    void previousError();
    void nextWarning();
    void previousWarning();
    void nextNotice();
    void previousNotice();

    void scrollToBeginning();
    void scrollToEnd();

    void selectNextMatching(QString text);
    void selectPreviousMatching(QString text);
private slots:
    void autoScroll(const QModelIndex &parent, int first, int last);
    void scrollToSelected();
private:
    QTimer m_autoScroll;

    void nextSeverity(LogSeverity);
    void previousSeverity(LogSeverity);
};

#endif // LOGVIEW_H
