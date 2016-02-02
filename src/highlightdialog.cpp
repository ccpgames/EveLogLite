#include "highlightdialog.h"
#include "ui_highlights.h"
#include "filterhighlight.h"

HighlightDialog::HighlightDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Highlights)
{
    ui->setupUi(this);

    m_highlightSets = HighlightSet::getSets();
    for( auto it = m_highlightSets.begin(); it != m_highlightSets.end(); ++it)
    {
        ui->highlightSets->addItem(it->m_name);
    }
    if (!m_highlightSets.empty())
    {
        ui->highlightSets->setCurrentIndex(0);
    }

    highlightSetSelected(ui->highlightSets->currentIndex());

    connect(ui->highlightSets, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &HighlightDialog::highlightSetSelected);
    connect(ui->add, &QPushButton::clicked, this, &HighlightDialog::addHighlightSet);
    connect(ui->remove, &QPushButton::clicked, this, &HighlightDialog::removeHighlightSet);
    connect(ui->name, &QLineEdit::textChanged, this, &HighlightDialog::nameChanged);
    connect(ui->addHighlight, &QPushButton::clicked, this, &HighlightDialog::addHighlight);
}

HighlightDialog::~HighlightDialog()
{
    delete ui;
}

void HighlightDialog::selectHighlight(QString name)
{
    for (int i = 0; i < ui->highlightSets->count(); ++i)
    {
        if (ui->highlightSets->itemText(i) == name)
        {
            ui->highlightSets->setCurrentIndex(i);
            highlightSetSelected(i);
            return;
        }
    }
}

const QVector<HighlightSet>& HighlightDialog::getHighlightSets() const
{
    return m_highlightSets;
}

void HighlightDialog::highlightSetSelected(int index)
{
    if (index < 0)
    {
        ui->name->setEnabled(false);
        ui->remove->setEnabled(false);
        ui->highlights->setEnabled(false);
        return;
    }
    ui->name->setEnabled(true);
    ui->remove->setEnabled(true);
    ui->highlights->setEnabled(true);

    ui->name->setText(m_highlightSets[index].m_name);

    for (auto it = std::begin(m_highlightWidgets); it != std::end(m_highlightWidgets); ++it)
    {
        delete *it;
    }
    m_highlightWidgets.clear();

    for (auto it = std::begin(m_highlightSets[index].m_highlights); it != std::end(m_highlightSets[index].m_highlights); ++it)
    {
        auto highlight = new FilterHighlight(ui->highlightsAreaContents);
        highlight->setHighlight(*it);
        connect(highlight, &FilterHighlight::changed, this, &HighlightDialog::highlightChanged);
        connect(highlight, &FilterHighlight::removed, this, &HighlightDialog::highlightRemoved);
        m_highlightWidgets.push_back(highlight);
        static_cast<QBoxLayout*>(ui->highlightsAreaContents->layout())->insertWidget(ui->highlightsAreaContents->layout()->count() - 1, highlight);
    }
}

void HighlightDialog::addHighlightSet()
{
    m_highlightSets.push_back(HighlightSet());
    m_highlightSets.back().m_name = "New Highlight";
    ui->highlightSets->addItem("New Highlight");
    ui->highlightSets->setCurrentIndex(ui->highlightSets->count() - 1);
}

void HighlightDialog::removeHighlightSet()
{
    auto index = ui->highlightSets->currentIndex();
    if (index >= 0)
    {
        m_highlightSets.removeAt(index);
        ui->highlightSets->removeItem(index);
        highlightSetSelected(ui->highlightSets->currentIndex());
    }
}

void HighlightDialog::nameChanged()
{
    auto index = ui->highlightSets->currentIndex();
    if (index >= 0)
    {
        m_highlightSets[index].m_name = ui->name->text();
        auto& filter = m_highlightSets[index];
        filter.m_name = ui->name->text();
        ui->highlightSets->setItemText(index, filter.m_name);
    }
}

void HighlightDialog::addHighlight()
{
    auto index = ui->highlightSets->currentIndex();
    if (index < 0)
    {
        return;
    }

    auto highlight = new FilterHighlight(ui->highlightsAreaContents);
    highlight->setHighlight(HighlightSet::Highlight());
    connect(highlight, &FilterHighlight::changed, this, &HighlightDialog::highlightChanged);
    connect(highlight, &FilterHighlight::removed, this, &HighlightDialog::highlightRemoved);
    m_highlightSets[index].m_highlights.push_back(HighlightSet::Highlight());
    m_highlightWidgets.push_back(highlight);
    static_cast<QBoxLayout*>(ui->highlightsAreaContents->layout())->insertWidget(ui->highlightsAreaContents->layout()->count() - 1, highlight);
}

void HighlightDialog::highlightRemoved(FilterHighlight* highlight)
{
    auto index = ui->highlightSets->currentIndex();
    if (index < 0)
    {
        return;
    }

    auto h = m_highlightWidgets.indexOf(highlight);
    m_highlightSets[index].m_highlights.removeAt(h);
    highlight->deleteLater();
}

void HighlightDialog::highlightChanged(FilterHighlight* highlight)
{
    auto index = ui->highlightSets->currentIndex();
    if (index < 0)
    {
        return;
    }

    auto h = m_highlightWidgets.indexOf(highlight);
    m_highlightSets[index].m_highlights[h] = highlight->highlight();
}
