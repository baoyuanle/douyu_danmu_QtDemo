#ifndef SINGLECAR_H
#define SINGLECAR_H
#include <QThread>
#include <QMap>
#include <QList>
#include <QMutex>
#include <atomic>
#include "danmuconfig.h"
#define RANK_SCar_NUM 100 //独轮车排行数量
class SCchachClear:public QThread
{
    Q_OBJECT
public:
    SCchachClear(QObject *pParent = nullptr);
    void stop();
protected:
    void run()override;
private:
    std::atomic<bool> m_bRun{true};
};

class singleCar:public QObject
{
    Q_OBJECT
public:
    singleCar(QObject *pParent = nullptr);
    ~singleCar();
    QList<ChatMsg> getRank();
signals:
    void sigUpdateRank();
private slots:
    void onNewMsg(const QMap<QString, QString> &);
private:
    void updateRank(const ChatMsg &msg);
private:
    ChatMsg m_lsRank[RANK_SCar_NUM];
    SCchachClear *m_pCleaner{nullptr};
};

#endif // SINGLECAR_H
