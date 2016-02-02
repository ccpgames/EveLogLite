#include "filterhighlight.h"
#include "ui_filterhighlight.h"
#include <QColorDialog>

FilterHighlight::FilterHighlight(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FilterHighlight)
{
    ui->setupUi(this);
    ui->conditions->createControls();
    connect(ui->foregroundOn, &QCheckBox::clicked, this, &FilterHighlight::foregroundEnabled);
    connect(ui->foreground, &QPushButton::clicked, this, &FilterHighlight::foregroundColorClicked);
    connect(ui->backgroundOn, &QCheckBox::clicked, this, &FilterHighlight::backgroundEnabled);
    connect(ui->background, &QPushButton::clicked, this, &FilterHighlight::backgroundColorClicked);
    connect(ui->conditions, &FilterConditions::changed, this, &FilterHighlight::conditionsChanged);
    connect(ui->remove, &QPushButton::clicked, this, &FilterHighlight::removeClicked);
    connect(ui->junctureAnd, &QRadioButton::clicked, this, &FilterHighlight::junctureChanged);
    connect(ui->junctureOr, &QRadioButton::clicked, this, &FilterHighlight::junctureChanged);
}

FilterHighlight::~FilterHighlight()
{
    delete ui;
}

void FilterHighlight::setHighlight(const HighlightSet::Highlight& highlight)
{
    m_highlight = highlight;
    m_foreground = m_highlight.m_foreground.isValid() ? m_highlight.m_foreground : QColor(0, 0, 0);
    m_background = m_highlight.m_background.isValid() ? m_highlight.m_background : QColor(255, 255, 255);
    if (m_highlight.m_juncture == Filter::AND)
    {
        ui->junctureAnd->setChecked(true);
    }
    else
    {
        ui->junctureOr->setChecked(true);
    }
    changeButtonIcon(ui->foreground, m_foreground);
    changeButtonIcon(ui->background, m_background);
    ui->foregroundOn->setChecked(m_highlight.m_foreground.isValid());
    ui->foreground->setEnabled(m_highlight.m_foreground.isValid());
    ui->backgroundOn->setChecked(m_highlight.m_background.isValid());
    ui->background->setEnabled(m_highlight.m_background.isValid());
    ui->conditions->setConditions(&m_highlight.m_conditions);
}

void FilterHighlight::changeButtonIcon(QPushButton* button, QColor color)
{
    auto size = button->iconSize();
    QPixmap icon(size);
    icon.fill(color);
    button->setIcon(QIcon(icon));
}

const HighlightSet::Highlight& FilterHighlight::highlight() const
{
    return m_highlight;
}

QSize FilterHighlight::sizeHint() const
{
    return QSize(width(), QFrame::sizeHint().height());
}

void FilterHighlight::foregroundEnabled(bool enabled)
{
    if (enabled)
    {
        m_highlight.m_foreground = m_foreground;
    }
    else
    {
        m_highlight.m_foreground = QColor();
    }
    ui->foreground->setEnabled(enabled);
}

void FilterHighlight::foregroundColorClicked()
{
    auto color = QColorDialog::getColor(m_highlight.m_foreground);
    if (color.isValid())
    {
        m_foreground = color;
        m_highlight.m_foreground = color;
    }
    changeButtonIcon(ui->foreground, m_highlight.m_foreground);
    emit changed(this);
}

void FilterHighlight::backgroundEnabled(bool enabled)
{
    if (enabled)
    {
        m_highlight.m_background = m_background;
    }
    else
    {
        m_highlight.m_background = QColor();
    }
    ui->background->setEnabled(enabled);
}

void FilterHighlight::backgroundColorClicked()
{
    auto color = QColorDialog::getColor(m_highlight.m_background);
    if (color.isValid())
    {
        m_background = color;
        m_highlight.m_background = color;
    }
    changeButtonIcon(ui->background, m_highlight.m_background);
    emit changed(this);
}

void FilterHighlight::conditionsChanged()
{
    adjustSize();
    emit changed(this);
}

void FilterHighlight::removeClicked()
{
    emit removed(this);
}

void FilterHighlight::junctureChanged()
{
    m_highlight.m_juncture = ui->junctureAnd->isChecked() ? Filter::AND : Filter::OR;
    emit changed(this);
}
