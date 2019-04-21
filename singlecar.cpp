#include "singlecar.h"

QMap<unsigned int,QMap<QString,ChatMsg>> g_mapUser;//uid, txt, ChatMsg
QMutex g_mt4map;
singleCar::singleCar(QObject *pParent)
    :QObject(pParent)
{
    m_pCleaner = new SCchachClear(this);
}

singleCar::~singleCar()
{
    m_pCleaner->stop();
    m_pCleaner->wait();
}

QList<ChatMsg> singleCar::getRank()
{
    QList<ChatMsg> lsRank;
    //for(auto &msg:m_lsRank){
    //    qDebug()<<"lsRank: "<<msg.txt<<"cnt:"<<msg.count;
    //}
    //冒泡排序
    ChatMsg msgTmp;
    for (int i=0; i<RANK_SCar_NUM;++i) {
        for (int j=i;j<RANK_SCar_NUM;++j) {
            if(m_lsRank[i].count<m_lsRank[j].count){
                msgTmp=m_lsRank[i];
                m_lsRank[i]=m_lsRank[j];
                m_lsRank[j]=msgTmp;
            }
        }
    }
    for(auto &msg:m_lsRank){
        if(msg.count<1){
            break;
        }
        //qDebug()<<"lsRank, sorted: "<<msg.txt<<"cnt:"<<msg.count;
        lsRank.append(msg);
    }
    return lsRank;
}

void singleCar::onNewMsg(const QMap<QString, QString> &massage)
{
    //qDebug()<<"onNewMsg Thread id:"<<QThread::currentThreadId();
    if(QString("chatmsg") != massage["type"])
    {
        return;
    }
    ChatMsg msg;
    msg.uid = massage["uid"].toUInt();
    msg.nn = massage["nn"];
    msg.txt = massage["txt"];
    msg.bnn = massage["bnn"];
    msg.level = massage["level"].toUInt();
    msg.nl = massage["nl"].toUInt();
    msg.bl = massage["bl"].toUInt();
    msg.tmRecent = time(nullptr);
    if(msg.nn.isEmpty() || msg.txt.isEmpty())
    {
        qDebug()<<"err,empty msg :"<<msg.nn<<","<<msg.txt<<","<<msg.uid<<
                  ","<<msg.bnn<<","<<msg.level<<","<<msg.nl<<","<<msg.bl;
        return;
    }
    int iUserCount = 0;
    //int iMsgCount = 0;
    {
        QMutexLocker lock(&g_mt4map);
        if(g_mapUser.cend() != g_mapUser.find(msg.uid)){
            //qDebug()<<"uid found, msg:"<<msg.txt;
            auto &mapMsg = g_mapUser[msg.uid];
            if(mapMsg.cend()!= mapMsg.find(msg.txt)){
                ++(mapMsg[msg.txt].count);
                //qDebug()<<"txt found, count:"<<mapMsg[msg.txt].count;
                updateRank(mapMsg[msg.txt]);
            }else {
                msg.count=1;
                mapMsg[msg.txt]=msg;
            }
            //qDebug()<<"msg cout:"<<mapMsg.size();
        }else{
            QMap<QString, ChatMsg> map;
            msg.count=1;
            map[msg.txt]=msg;
            g_mapUser[msg.uid] = map;
        }
        iUserCount = g_mapUser.size();
    }
    if(0 == iUserCount%50){
        qDebug()<<"user cout:"<<iUserCount;
    }
    if(iUserCount>100 && !m_pCleaner->isRunning()){
        m_pCleaner->start();
    }
}

void singleCar::updateRank(const ChatMsg &msg)
{
    int iLowestIdx=0;
    for(int i=0;i<RANK_SCar_NUM; ++i)
    {
        //当前弹幕已在榜单内，直接更新该条
        if(msg.txt == m_lsRank[i].txt)
        {
            iLowestIdx = i;
            break;
        }
        //不在榜单，找到榜单最低名次比较
        if(m_lsRank[i].count<m_lsRank[iLowestIdx].count)
        {
            iLowestIdx = i;
        }
    }
    if(msg.count>m_lsRank[iLowestIdx].count)
    {
        m_lsRank[iLowestIdx]=msg;
        emit sigUpdateRank();
    }
}

SCchachClear::SCchachClear(QObject *pParent)
    :QThread(pParent)
{

}

void SCchachClear::stop()
{
    m_bRun = false;
}

void SCchachClear::run()
{
    const int iSleepS = 2*60;//缓存清理间隔 秒
    while(m_bRun){
        for (int i=0;i<iSleepS;++i) {
            if(!m_bRun)
                return;
            sleep(1);
        }
        {
            time_t tmNow = time(nullptr);
            int iClearCount=0;
            QMutexLocker lock(&g_mt4map);
            qDebug()<<"begin clear map---------count:"<<g_mapUser.count();
            for (auto iterUser = g_mapUser.begin();iterUser!= g_mapUser.end();) {
                auto &mapMsg = iterUser.value();
                for (auto iterMsg = mapMsg.begin();iterMsg!=mapMsg.end();) {
                    //todo这里可以改为小于rank里最小值的都删除
                    if(iterMsg.value().count<2 && (tmNow - iterMsg.value().tmRecent)>iSleepS)
                    {
                        iterMsg = mapMsg.erase(iterMsg);
                        ++iClearCount;
                    }
                    else {
                        ++iterMsg;
                    }
                }
                if(mapMsg.size()<1){
                    iterUser = g_mapUser.erase(iterUser);
                }else {
                    ++iterUser;
                }
            }
            qDebug()<<"end clear map---------cleard count:"<<iClearCount<<"map count:"<<g_mapUser.count();
        }
    }
}
