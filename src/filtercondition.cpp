#include "filtercondition.h"
#include <QBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>

FilterCondition::FilterCondition(QWidget *parent, const Filter::Condition& condition)
    :QWidget(parent),
      m_condition(condition)
{
    auto layout = new QBoxLayout(QBoxLayout::TopToBottom);
    auto formLayout = new QFormLayout();
    layout->addLayout(formLayout);
    auto hLayout = new QBoxLayout(QBoxLayout::LeftToRight);

    auto field = new QComboBox(this);
    field->addItem("Severity");
    field->addItem("Timestamp");
    field->addItem("PID");
    field->addItem("Executable");
    field->addItem("Machine");
    field->addItem("Module");
    field->addItem("Channel");
    field->addItem("Message");
    connect(field, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FilterCondition::fieldChanged);
    hLayout->addWidget(field);

    auto op = new QComboBox(this);
    op->addItem("equals");
    op->addItem("not equals");
    op->addItem("contains");
    op->addItem("does not contain");
    op->addItem("less than");
    op->addItem("less than or equals");
    op->addItem("greater than");
    op->addItem("greater than or equals");
    connect(op, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FilterCondition::operationChanged);
    hLayout->addWidget(op);

    m_value = new QLineEdit(this);
    hLayout->addWidget(m_value, 1);

    auto remove = new QPushButton("Remove", this);
    hLayout->addWidget(remove);
    connect(remove, &QPushButton::clicked, this, &FilterCondition::removeClicked);

    layout->addLayout(hLayout);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);

    field->setCurrentIndex(condition.m_field + 1);
    fieldChanged(condition.m_field + 1);
    op->setCurrentIndex(condition.m_op);
    if (condition.m_field == LOGFIELD_SEVERITY)
    {
        static_cast<QComboBox*>(m_value)->setCurrentIndex(condition.m_operand.toInt());
    }
    else
    {
        static_cast<QLineEdit*>(m_value)->setText(condition.m_operand.toString());
    }
}

const Filter::Condition& FilterCondition::condition() const
{
    return m_condition;
}

void FilterCondition::fieldChanged(int index)
{
    bool emitChanged = false;
    if (m_condition.m_field != index - 1)
    {
        if (index == 0)
        {
            m_condition.m_operand = 0;
        }
        else
        {
            m_condition.m_operand = "";
        }
        emitChanged = true;
    }
    m_condition.m_field = LogField(index - 1);
    if (index == 0)
    {
        auto value = new QComboBox(this);
        value->addItem("info");
        value->addItem("notice");
        value->addItem("warning");
        value->addItem("error");
        value->setCurrentIndex(m_condition.m_operand.toInt());
        connect(value, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FilterCondition::severityChanged);
        auto item = layout()->replaceWidget(m_value, value);
        delete item;
        delete m_value;
        m_value = value;
    }
    else
    {
        auto value = new QLineEdit(m_condition.m_operand.toString(), this);
        connect(value, &QLineEdit::textChanged, this, &FilterCondition::operandChanged);
        auto item = layout()->replaceWidget(m_value, value);
        delete item;
        delete m_value;
        m_value = value;
    }
    if (emitChanged)
    {
        emit changed(this);
    }
}

void FilterCondition::operationChanged(int index)
{
    if (m_condition.m_op != Filter::Operator(index))
    {
        m_condition.m_op = Filter::Operator(index);
        emit changed(this);
    }
}

void FilterCondition::severityChanged(int index)
{
    if (m_condition.m_operand != index)
    {
        m_condition.m_operand = index;
        emit changed(this);
    }
}

void FilterCondition::operandChanged(QString text)
{
    if (m_condition.m_operand != text)
    {
        m_condition.m_operand = text;
        emit changed(this);
    }
}

void FilterCondition::removeClicked()
{
    emit removed(this);
}
