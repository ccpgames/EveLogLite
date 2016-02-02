#include "overlaylayout.h"

OverlayLayout::OverlayLayout(QWidget *parent)
    :QLayout(parent)
{

}

OverlayLayout::~OverlayLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}


void OverlayLayout::addItem(QLayoutItem *item)
{
    list.append(item);
}

QSize OverlayLayout::sizeHint() const
{
    QSize s(0,0);
    int n = list.count();
    if (n > 0)
        s = QSize(100,70); //start with a nice default size
    int i = 0;
    while (i < n)
    {
        QLayoutItem *o = list.at(i);
        s = s.expandedTo(o->sizeHint());
        ++i;
    }
    return s;
}

QSize OverlayLayout::minimumSize() const
{
    QSize s(0,0);
    int n = list.count();
    int i = 0;
    while (i < n) {
        QLayoutItem *o = list.at(i);
        s = s.expandedTo(o->minimumSize());
        ++i;
    }
    return s;
}

int OverlayLayout::count() const
{
    return list.size();
}

QLayoutItem *OverlayLayout::itemAt(int index) const
{
    return list.value(index);
}

QLayoutItem *OverlayLayout::takeAt(int index)
{
    return index >= 0 && index < list.size() ? list.takeAt(index) : 0;
}

void OverlayLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    if (list.size() == 0)
        return;

    int i = 0;
    while (i < list.size())
    {
        QLayoutItem *o = list.at(i);
        if (i == 0)
        {
            o->setGeometry(rect);
        }
        else
        {
            auto size = o->sizeHint();
            QRect geom(rect.x() + rect.width() - size.width(), rect.y(), size.width(), size.height());
            o->setGeometry(geom);
        }
        ++i;
    }
}
