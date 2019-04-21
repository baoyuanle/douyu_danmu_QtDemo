#include "scarrankmodel.h"

SCarRankModel::SCarRankModel(QObject *parent)
    : QStandardItemModel(parent)
{
    QStringList strHeader;
    strHeader<<tr("用户")<<tr("弹幕")<<tr("独轮车速")<<tr("等级")<<tr("牌子")<<tr("时间");
    setHorizontalHeaderLabels(strHeader);
}

void SCarRankModel::add(const ChatMsg &msg)
{
    QVariant qvMsg;
    qvMsg.setValue(msg);
    auto pNN = new QStandardItem(msg.nn);
    pNN->setData(qvMsg, Qt::UserRole+1);
    auto pTxt = new QStandardItem(msg.txt);
    auto pCount = new QStandardItem(QString::number(msg.count));
    auto pLev = new QStandardItem(QString::number(msg.level));
    QString strBand = "白佩佩";
    if(msg.bl>0){
        strBand= msg.bnn+" "+QString::number(msg.bl);
    }
    auto pBand = new QStandardItem(strBand);
    QString strTime = QDateTime::fromTime_t(msg.tmRecent).toString("MM-dd hh:mm");
    auto pTime = new QStandardItem(strTime);
    QList<QStandardItem*> ls;
    ls<<pNN<<pTxt<<pCount<<pLev<<pBand<<pTime;
    for(auto& p:ls){
        p->setEditable(false);
    }
    appendRow(ls);
}

