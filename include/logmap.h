#ifndef LOGMAP_H
#define LOGMAP_H

#include <QWidget>
#include <QPixmap>

class LogFilter;
class QTableView;


class LogMap : public QWidget
{
    Q_OBJECT
public:
    explicit LogMap(QWidget *parent = 0);

    void setModel(LogFilter *model);
    void setBuddyView(QTableView *buddy);
protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent * event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
private:
    void paintMap();
    void scrollTo(int y);

    LogFilter *m_model;
    QPixmap m_map;
    QTableView *m_buddy;
    bool m_mapValid;
public slots:
    void invalidateMap();
};

#endif // LOGMAP_H
