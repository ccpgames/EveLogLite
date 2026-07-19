#ifndef FILTERCONDITION_H
#define FILTERCONDITION_H

#include <QWidget>
#include "logfilter.h"

class FilterCondition : public QWidget
{
    Q_OBJECT
public:
    FilterCondition(QWidget *parent, const Filter::Condition& condition);

    const Filter::Condition& condition() const;
signals:
    void removed(FilterCondition*);
    void changed(FilterCondition*);
public slots:
    void fieldChanged(int index);
    void operationChanged(int index);
    void severityChanged(int index);
    void operandChanged(QString text);
    void removeClicked();
private:
    Filter::Condition m_condition;
    QWidget* m_value;
};

#endif // FILTERCONDITION_H
