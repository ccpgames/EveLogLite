#include "logmap.h"
#include "logfilter.h"
#include "abstractlogmodel.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QTableView>

LogMap::LogMap(QWidget *parent) : QWidget(parent),
    m_model(nullptr),
    m_buddy(nullptr),
    m_mapValid(false)
{
    m_map = QPixmap(width(), height());
}

void LogMap::setModel(LogFilter *model)
{
    if (m_model)
    {
        disconnect(m_model, &LogFilter::rowsInserted, this, &LogMap::invalidateMap);
        disconnect(m_model, &LogFilter::rowsRemoved, this, &LogMap::invalidateMap);
    }
    m_model = model;
    if (m_model)
    {
        connect(m_model, &LogFilter::rowsInserted, this, &LogMap::invalidateMap);
        connect(m_model, &LogFilter::rowsRemoved, this, &LogMap::invalidateMap);
    }
}

void LogMap::setBuddyView(QTableView *buddy)
{
    m_buddy = buddy;
}

void LogMap::resizeEvent(QResizeEvent * event)
{
    m_map = QPixmap(width(), height());
    invalidateMap();
    QWidget::resizeEvent(event);
}

void LogMap::invalidateMap()
{
    m_mapValid = false;
    update();
}

void LogMap::paintMap()
{
    QPainter p(&m_map);
    p.fillRect(rect(), QBrush(QApplication::palette().window().color()));
    if (!m_model)
    {
        return;
    }

    QPen warning(QBrush(Qt::yellow), 3, Qt::SolidLine);
    QPen error(QBrush(Qt::red), 3, Qt::SolidLine);
    QPen shadow(QBrush(QApplication::palette().shadow().color()), 3, Qt::SolidLine);

    auto width = rect().width() - 2;
    auto height = rect().height();

    int count = m_model->rowCount();
    auto model = static_cast<AbstractLogModel*>(m_model->sourceModel());
    int prevY = -1;
    for (int i = 0; i < count; ++i)
    {
        auto row = m_model->mapToSource(m_model->index(i, 0)).row();
        auto message = model->message(row);
        if (!message)
        {
            continue;
        }
        if (message->severity >= SEVERITY_WARN)
        {
            float y = 3 + float(i) / count * (height - 6);
            if (int(y) != prevY)
            {
                QLineF line(2, y + 1, width, y + 1);
                p.setPen(shadow);
                p.drawLine(line);
                p.setPen(message->severity == SEVERITY_WARN ? warning : error);
                line = QLineF(1, y, width - 1, y);
                p.drawLine(line);
                prevY = int(y);
            }
        }
    }
}

void LogMap::paintEvent(QPaintEvent *)
{
    if (!m_mapValid)
    {
        paintMap();
        m_mapValid = true;
    }
    QPainter p(this);
    p.drawPixmap(0, 0, m_map);
}

void LogMap::scrollTo(int y)
{
    if (m_buddy && m_model)
    {
        auto height = rect().height();
        int count = m_model->rowCount();
        int index = int(count * float(y - 3) / (height - 6));
        m_buddy->scrollTo(m_model->index(index, 0), QAbstractItemView::PositionAtCenter);
    }
}

void LogMap::mousePressEvent(QMouseEvent *event)
{
    scrollTo(event->pos().y());
    QWidget::mousePressEvent(event);
}

void LogMap::mouseMoveEvent(QMouseEvent *event)
{
    scrollTo(event->pos().y());
    QWidget::mouseMoveEvent(event);
}
