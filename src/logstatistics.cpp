#include "logstatistics.h"
#include "ui_logstatistics.h"
#include "abstractlogmodel.h"
#include "logmodel.h"

LogStatistics::LogStatistics(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogStatistics),
    m_model(nullptr)
{
    ui->setupUi(this);
    updateStatistics();
}

LogStatistics::~LogStatistics()
{
    delete ui;
}

void LogStatistics::setModel(AbstractLogModel *model)
{
    if (m_model)
    {
        disconnect(m_model, &AbstractLogModel::rowsInserted, this, &LogStatistics::updateStatistics);
        disconnect(m_model, &AbstractLogModel::rowsRemoved, this, &LogStatistics::updateStatistics);
        if (auto logModel = dynamic_cast<LogModel*>(model))
        {
            disconnect(logModel, &LogModel::clientConnected, this, &LogStatistics::updateStatistics);
            disconnect(logModel, &LogModel::clientDisconnected, this, &LogStatistics::updateStatistics);
        }
    }
    m_model = model;
    if (m_model)
    {
        connect(m_model, &AbstractLogModel::rowsInserted, this, &LogStatistics::updateStatistics);
        connect(m_model, &AbstractLogModel::rowsRemoved, this, &LogStatistics::updateStatistics);
        if (auto logModel = dynamic_cast<LogModel*>(model))
        {
            connect(logModel, &LogModel::clientConnected, this, &LogStatistics::updateStatistics);
            connect(logModel, &LogModel::clientDisconnected, this, &LogStatistics::updateStatistics);
        }
    }
    updateStatistics();
}

void LogStatistics::updateStatistics()
{
    AbstractLogModel::Statistics statistics;
    if (m_model)
    {
        statistics = m_model->statistics();
        ui->serverAlive->setEnabled(m_model->isListening());
    }
    else
    {
        statistics.error = 0;
        statistics.warning = 0;
        statistics.notice = 0;
        statistics.info = 0;
        statistics.clients = 0;
        ui->serverAlive->setEnabled(false);
    }
    ui->errorCount->setText(QString::number(statistics.error));
    ui->warningCount->setText(QString::number(statistics.warning));
    ui->noticeCount->setText(QString::number(statistics.notice));
    ui->infoCount->setText(QString::number(statistics.info));
    ui->serverAlive->setToolTip(QString("%1 clients connected").arg(QString::number(statistics.clients)));
}
