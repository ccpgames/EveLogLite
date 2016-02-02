#ifndef FILTERHIGHLIGHT_H
#define FILTERHIGHLIGHT_H

#include <QFrame>
#include "logfilter.h"

namespace Ui {
class FilterHighlight;
}

class QPushButton;

class FilterHighlight : public QFrame
{
    Q_OBJECT

public:
    explicit FilterHighlight(QWidget *parent = 0);
    ~FilterHighlight();

    void setHighlight(const HighlightSet::Highlight& highlight);
    const HighlightSet::Highlight& highlight() const;
    virtual QSize sizeHint() const;
private:
    void changeButtonIcon(QPushButton* button, QColor color);

    Ui::FilterHighlight *ui;
    HighlightSet::Highlight m_highlight;
    QColor m_foreground;
    QColor m_background;
signals:
    void removed(FilterHighlight*);
    void changed(FilterHighlight*);
private slots:
    void foregroundEnabled(bool enabled);
    void foregroundColorClicked();
    void backgroundEnabled(bool enabled);
    void backgroundColorClicked();
    void conditionsChanged();
    void removeClicked();
    void junctureChanged();
};

#endif // FILTERHIGHLIGHT_H
