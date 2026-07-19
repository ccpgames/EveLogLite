#ifndef FIXEDHEADER_H
#define FIXEDHEADER_H

#include <QHeaderView>

class FixedHeader : public QHeaderView
{
public:
    FixedHeader(Qt::Orientation orientation, QWidget * parent = 0);
protected:
    virtual QSize sizeHint() const;
};

#endif // FIXEDHEADER_H
