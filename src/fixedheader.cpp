#include "fixedheader.h"

FixedHeader::FixedHeader(Qt::Orientation orientation, QWidget * parent)
    :QHeaderView(orientation, parent)
{

}

QSize FixedHeader::sizeHint() const
{
    return QSize(64, 0);
}
