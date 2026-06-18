#ifndef FILTER_H
#define FILTER_H

#include <QDialog>
#include "abstractlogmodel.h"
#include "logfilter.h"

namespace Ui {
class Filter;
}

class FilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterDialog(QWidget *parent = 0);
    ~FilterDialog();

    void selectFilter(QString name);

    const QVector<Filter>& getFilters() const;
private:
    Ui::Filter *ui;
    QVector<Filter> m_filters;
private slots:
    void filterSelected(int index);
    void newFilter();
    void removeFilter();
    void filterNameChanged();
    void filterJunctureChanged();
};

#endif // FILTER_H
