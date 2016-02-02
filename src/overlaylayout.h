#ifndef OVERLAYLAYOUT_H
#define OVERLAYLAYOUT_H

#include <QLayout>


class OverlayLayout: public QLayout
{
public:
    OverlayLayout(QWidget *parent);
    ~OverlayLayout();

    void addItem(QLayoutItem *item);
    QSize sizeHint() const;
    QSize minimumSize() const;
    int count() const;
    QLayoutItem *itemAt(int) const;
    QLayoutItem *takeAt(int);
    void setGeometry(const QRect &rect);
private:
    QList<QLayoutItem*> list;
};

#endif // OVERLAYLAYOUT_H
