#ifndef SCARRANKMODEL_H
#define SCARRANKMODEL_H

#include <QStandardItemModel>
#include "danmuconfig.h"

class SCarRankModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit SCarRankModel(QObject *parent = nullptr);
    void add(const ChatMsg &msg);
private:
};

#endif // SCARRANKMODEL_H
