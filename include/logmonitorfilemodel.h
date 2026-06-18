#ifndef LOGMONITORFILEMODEL_H
#define LOGMONITORFILEMODEL_H

#include "abstractlogmodel.h"


class LogMonitorFileModel : public AbstractLogModel
{
public:
    LogMonitorFileModel(const QString &dbPath, QObject *parent = nullptr);

    const Statistics &statistics() const;
    bool isListening() const;

    static bool saveModel(AbstractLogModel *model, const QString& fileName);
private:
    Statistics m_statistics;
public slots:
    void clear();
};

#endif // LOGMONITORFILEMODEL_H
