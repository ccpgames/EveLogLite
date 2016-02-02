#ifndef FILTERCONDITIONS_H
#define FILTERCONDITIONS_H

#include <QWidget>
#include "logfilter.h"

class QPushButton;
class FilterCondition;

class FilterConditions : public QWidget
{
    Q_OBJECT

public:
    FilterConditions(QWidget* parent = nullptr);

    void createControls();

    void setConditions(Filter::Conditions* conditions);
    Filter::Conditions* conditions() const;

signals:
    void changed();
private slots:
    void addCondition();
    void conditionRemoved(FilterCondition* condition);
    void filterConditionChanged(FilterCondition* condition);
private:
    Filter::Conditions* m_conditions;
    QVector<FilterCondition*> m_conditionWidgets;
    QPushButton* m_add;
};

#endif // FILTERCONDITIONS_H
