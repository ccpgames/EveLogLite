#ifndef LOGSTATISTICS_H
#define LOGSTATISTICS_H

#include <QWidget>

class AbstractLogModel;

namespace Ui {
class LogStatistics;
}


class LogStatistics : public QWidget
{
    Q_OBJECT

public:
    explicit LogStatistics(QWidget *parent = 0);
    ~LogStatistics();

    void setModel(AbstractLogModel *model);
private:
    Ui::LogStatistics *ui;
    AbstractLogModel *m_model;
private slots:
    void updateStatistics();
};

#endif // LOGSTATISTICS_H
