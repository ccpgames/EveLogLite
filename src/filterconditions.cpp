#include "filterconditions.h"
#include <QBoxLayout>
#include <QPushButton>
#include "filtercondition.h"

FilterConditions::FilterConditions(QWidget* parent)
    : QWidget(parent),
      m_conditions(nullptr)
{
    setEnabled(false);
}

void FilterConditions::createControls()
{
    if (!layout())
    {
        setLayout(new QVBoxLayout());
    }
    m_add = new QPushButton("Add Condition", this);
    connect(m_add, &QPushButton::clicked, this, &FilterConditions::addCondition);
    layout()->addWidget(m_add);
}

void FilterConditions::setConditions(Filter::Conditions* conditions)
{
    m_conditions = conditions;
    setEnabled(m_conditions != nullptr);

    for (auto it = std::begin(m_conditionWidgets); it != std::end(m_conditionWidgets); ++it)
    {
        delete *it;
    }
    m_conditionWidgets.clear();

    if (m_conditions)
    {
        for (int i = 0; i < m_conditions->size(); ++i)
        {
            auto c = new FilterCondition(this, m_conditions->at(i));
            connect(c, &FilterCondition::removed, this, &FilterConditions::conditionRemoved);
            connect(c, &FilterCondition::changed, this, &FilterConditions::filterConditionChanged);
            auto boxLayout = static_cast<QBoxLayout*>(layout());
            boxLayout->insertWidget(i, c);
            m_conditionWidgets.push_back(c);
        }
    }
}

Filter::Conditions* FilterConditions::conditions() const
{
    return m_conditions;
}

void FilterConditions::addCondition()
{
    if (!m_conditions)
    {
        return;
    }
    m_conditions->push_back(Filter::Condition());
    auto c = new FilterCondition(this, m_conditions->back());
    connect(c, &FilterCondition::removed, this, &FilterConditions::conditionRemoved);
    connect(c, &FilterCondition::changed, this, &FilterConditions::filterConditionChanged);
    auto boxLayout = static_cast<QBoxLayout*>(layout());
    boxLayout->insertWidget(boxLayout->count() - 1, c);
    m_conditionWidgets.push_back(c);
    adjustSize();
    emit changed();
}

void FilterConditions::conditionRemoved(FilterCondition* condition)
{
    int index = m_conditionWidgets.indexOf(condition);
    m_conditions->removeAt(index);
    condition->deleteLater();
    m_conditionWidgets.removeAt(index);
    adjustSize();
    emit changed();
}

void FilterConditions::filterConditionChanged(FilterCondition* condition)
{
    int index = m_conditionWidgets.indexOf(condition);
    (*m_conditions)[index] = condition->condition();
    emit changed();
}
