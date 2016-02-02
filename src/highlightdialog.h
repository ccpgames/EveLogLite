#ifndef HIGHLIGHTS_H
#define HIGHLIGHTS_H

#include <QDialog>
#include "logfilter.h"

namespace Ui {
class Highlights;
}

class FilterHighlight;

class HighlightDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HighlightDialog(QWidget *parent = 0);
    ~HighlightDialog();

    void selectHighlight(QString name);

    const QVector<HighlightSet>& getHighlightSets() const;
private slots:
    void highlightSetSelected(int index);
    void addHighlightSet();
    void removeHighlightSet();
    void nameChanged();

    void addHighlight();
    void highlightRemoved(FilterHighlight* highlight);
    void highlightChanged(FilterHighlight* highlight);
private:
    Ui::Highlights *ui;
    QVector<HighlightSet> m_highlightSets;
    QVector<FilterHighlight*> m_highlightWidgets;
};

#endif // HIGHLIGHTS_H
