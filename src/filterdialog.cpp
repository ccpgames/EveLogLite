#include "filterdialog.h"
#include "ui_filter.h"
#include "filtercondition.h"
#include "filterhighlight.h"
#include <QRegExpValidator>

FilterDialog::FilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Filter)
{
    ui->setupUi(this);
    ui->filterAreaContents->createControls();
    ui->name->setValidator(new QRegExpValidator(QRegExp("[\\w ]+"), this));

    m_filters = Filter::getFilters();
    for( auto it = m_filters.begin(); it != m_filters.end(); ++it)
    {
        ui->filters->addItem(it->m_name);
    }
    if (!m_filters.empty())
    {
        ui->filters->setCurrentIndex(0);
    }

    filterSelected(ui->filters->currentIndex());

    connect(ui->filters, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FilterDialog::filterSelected);
    connect(ui->add, &QPushButton::clicked, this, &FilterDialog::newFilter);
    connect(ui->remove, &QPushButton::clicked, this, &FilterDialog::removeFilter);
    connect(ui->name, &QLineEdit::textChanged, this, &FilterDialog::filterNameChanged);
    connect(ui->junctureAnd, &QRadioButton::clicked, this, &FilterDialog::filterJunctureChanged);
    connect(ui->junctureOr, &QRadioButton::clicked, this, &FilterDialog::filterJunctureChanged);
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

void FilterDialog::selectFilter(QString name)
{
    for (int i = 0; i < ui->filters->count(); ++i)
    {
        if (ui->filters->itemText(i) == name)
        {
            ui->filters->setCurrentIndex(i);
            filterSelected(i);
            return;
        }
    }
}

const QVector<Filter>& FilterDialog::getFilters() const
{
    return m_filters;
}

void FilterDialog::filterSelected(int index)
{
    if (index < 0)
    {
        ui->name->setEnabled(false);
        ui->remove->setEnabled(false);
        ui->groupBox->setEnabled(false);
        return;
    }
    ui->name->setEnabled(true);
    ui->remove->setEnabled(true);
    ui->groupBox->setEnabled(true);

    ui->name->setText(m_filters[index].m_name);
    if (m_filters[index].m_juncture == Filter::AND)
    {
        ui->junctureAnd->setChecked(true);
    }
    else
    {
        ui->junctureOr->setChecked(true);
    }

    ui->filterAreaContents->setConditions(&m_filters[index].m_conditions);
}

void FilterDialog::newFilter()
{
    m_filters.push_back(Filter());
    m_filters.back().m_name = "New Filter";
    ui->filters->addItem("New Filter");
    ui->filters->setCurrentIndex(ui->filters->count() - 1);
}

void FilterDialog::removeFilter()
{
    auto index = ui->filters->currentIndex();
    if (index >= 0)
    {
        m_filters.removeAt(index);
        ui->filters->removeItem(index);
        filterSelected(ui->filters->currentIndex());
    }
}

void FilterDialog::filterNameChanged()
{
    auto index = ui->filters->currentIndex();
    if (index >= 0)
    {
        m_filters[index].m_name = ui->name->text();
        auto& filter = m_filters[index];
        filter.m_name = ui->name->text();
        ui->filters->setItemText(index, filter.m_name);
    }
}

void FilterDialog::filterJunctureChanged()
{
    auto index = ui->filters->currentIndex();
    if (index < 0)
    {
        return;
    }

    m_filters[index].m_juncture = ui->junctureAnd->isChecked() ? Filter::AND : Filter::OR;
}
