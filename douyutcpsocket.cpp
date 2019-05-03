#include "douyutcpsocket.h"
#include "danmuconfig.h"
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>
#include <QDataStream>
#include <thread>
#include <QMetaType>

DouyuTcpSocket::DouyuTcpSocket(QObject *parent)
    :QObject(parent)
{
    qRegisterMetaType<QMap<QString, QString>>("QMap<QString, QString>");
    m_uPort = _Douyu_DanmuServer_Port;
}

DouyuTcpSocket::~DouyuTcpSocket()
{
    if(pTcpDanmuSoc){
        pTcpDanmuSoc->abort();
    }
}

void DouyuTcpSocket::setPort(unsigned short port)
{
    m_uPort = port;
}


void DouyuTcpSocket::loginAuth()
{
    QStringList key_list = (QStringList()
                            <<"type"
                            //<<"username"
                            //<<"password"
                            <<"roomid"
                            //<<"rid"
                            //<<"gid"
                            );
    QStringList value_list = (QStringList()
                              <<"loginreq" //登录请求
                              //<<""
                              //<<""
                              //<<danmu_rid
                              <<danmu_rid //房间号
                              //<<_Douyu_Room_gid //分组
                );
    QString content = STTSerialization(key_list,value_list);
    this->messageWrite(content);
    request_state = "loginReq";
}

void DouyuTcpSocket::readDanmuMessage()
{
    QByteArray inBlock = pTcpDanmuSoc->readAll(); //接收数据块
    QString content;
    int pos = 0;
    while((pos = inBlock.indexOf(QString("type"),pos)) != -1)
    {
        content = inBlock.mid(pos);
        QMap<QString,QString> messageMap = STTDeserialization(content);
        //弹幕类型分析
        if(messageMap["type"] == QString("loginres"))
        {//出现表示服务端消息已经发完，可进入下一个步骤
            request_state = "joingroup";
            qDebug("loginres!");
        }

        if(messageMap["type"] == QString("chatmsg")||
                messageMap["type"] == QString("dgb"))//||//赠送礼物
                //messageMap["type"] == QString("bc_buy_deserve"))//用户赠送酬勤通知消息
        {
            emit chatMessage(messageMap);
        }
        pos = pos + content.length();
    }

    if(request_state == "joingroup")
    {
        QStringList key_list = (QStringList()
                                <<"type"
                                <<"rid"
                                <<"gid"
                                );
        QStringList value_list = (QStringList()
                                  <<"joingroup" //登录请求
                                  <<danmu_rid //房间号
                                  <<_Douyu_Room_gid //分组
                    );

        QString content = STTSerialization(key_list,value_list);
        this->messageWrite(content);
        request_state = "receiveDanmu";
        qDebug()<<"joingroup";
        if(nullptr == timerHeart){
            timerHeart = new QTimer(this);
            connect(timerHeart,SIGNAL(timeout()),this,SLOT(keepAlive()));
        }
        timerHeart->start(_Douyu_DanmuServer_Intervals);
    }
}

void DouyuTcpSocket::messageWrite(QString &content)
{
    qDebug()<<"messageWrite:"<<content;
    const char *content_ptr = content.toStdString().c_str();
    char *pCont = new char[content.length()+1];
    memcpy(pCont, content_ptr, content.length());
    pCont[content.length()]=0;
    QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
    qint32 length = 4 + 4 + content.length() + 1;// 2个uint32字段长度+内容长度+'\0'
    sendOut<<qint32(hexReverse_qint32(length))<<qint32(hexReverse_qint32(length))<<qint32(_Douyu_CTS_Num);
    outBlock.append(pCont,content.length());
    outBlock.append('\0');
    auto iWrited = pTcpDanmuSoc->write(outBlock);
    qDebug()<<"iWrited:"<<iWrited;
    qDebug()<<"iWrited:"<<outBlock;
    outBlock.clear();
    delete []pCont;
}

void DouyuTcpSocket::connectDanmuServer()
{
    qDebug()<<"connectDanmuServer currentThreadId:"<<QThread::currentThreadId();
    request_state="";
    if(nullptr == pTcpDanmuSoc){
        pTcpDanmuSoc = new QTcpSocket(this);
        connect(pTcpDanmuSoc,SIGNAL(connected()),this,SLOT(loginAuth()));
        connect(pTcpDanmuSoc,SIGNAL(readyRead()),this,SLOT(readDanmuMessage()));
        connect(pTcpDanmuSoc,SIGNAL(error(QAbstractSocket::SocketError)),
                this,SLOT(displayError(QAbstractSocket::SocketError)));
        connect(pTcpDanmuSoc,SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this,SLOT(stateChanged(QAbstractSocket::SocketState)));
    }
    if(pTcpDanmuSoc->state() == QAbstractSocket::ConnectedState)
    {
        pTcpDanmuSoc->abort();
    }
    if(m_uConFailTime>=3){//连续x次失败后换备用端口
        m_uConFailTime = 0;
        m_uPort = (_Douyu_DanmuServer_Port_Backup == m_uPort)?_Douyu_DanmuServer_Port:_Douyu_DanmuServer_Port_Backup;
    }
    //pTcpDanmuSoc->connectToHost(_Douyu_DanmuServer_HostName, m_uPort);
    pTcpDanmuSoc->connectToHost("36.155.10.62", m_uPort);
    if (pTcpDanmuSoc->waitForConnected(2000)) {
        qDebug("Connected!");
        m_uConFailTime=0;
    }
    else {
        pTcpDanmuSoc->abort();
        qDebug("aborted!");
        ++m_uConFailTime;
    }
}

void DouyuTcpSocket::reconnect()
{
    qDebug()<<"reconnect...";
    if(nullptr == timerReconnect){
        timerReconnect = new QTimer(this);
        timerReconnect->setInterval(500);
        timerReconnect->setSingleShot(true);
        connect(timerReconnect, SIGNAL(timeout()), this, SLOT(onTimerRec()));
    }
    timerReconnect->start();
}

void DouyuTcpSocket::close()
{
    m_bAutoRecennect = false;
    if(pTcpDanmuSoc){
        pTcpDanmuSoc->abort();
    }
    if(timerHeart){
        timerHeart->stop();
    }
    if(timerReconnect){
        timerReconnect->stop();
    }
    request_state="";
}

void DouyuTcpSocket::displayError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString error_str = pTcpDanmuSoc->errorString();
    //tcpDanmuSoc.close();
    error_str = tr("网络错误：")+error_str;
    qDebug()<<error_str;
    if(m_bAutoRecennect){
        emit sigUpdateStat(error_str + tr(" 正在重连。。。"));
        reconnect();
    }else{
        emit sigUpdateStat(error_str);
    }
}

void DouyuTcpSocket::keepAlive()
{
    if(request_state == "receiveDanmu")
    {
        QStringList key_list = (QStringList()
                                <<"type"
                                );
        QStringList value_list = (QStringList()
                                  <<"mrkl" //登录请求
                    );
        QString content = STTSerialization(key_list,value_list);
        this->messageWrite(content);
    }
}

void DouyuTcpSocket::stateChanged(QAbstractSocket::SocketState state)
{
    qDebug()<<state;
    bool reConnect = false;
    QString str;
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        str = tr("未连接");
        reConnect=true;
        break;
    case QAbstractSocket::HostLookupState:
        str = tr("解析服务器。。。");
        break;
    case QAbstractSocket::ConnectingState:
        str = tr("连接中。。。");
        break;
    case QAbstractSocket::ConnectedState:
        str = tr("已连接");
        break;
    case QAbstractSocket::ClosingState:
        str = tr("断开连接中。。。");
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
    if(!str.isEmpty()){
        emit sigUpdateStat(str);
    }
    if(reConnect && m_bAutoRecennect){
        emit sigUpdateStat(tr("正在重连。。。"));
        reconnect();
    }
}

void DouyuTcpSocket::onConnect(const QString &room)
{
    m_bAutoRecennect = true;
    danmu_rid = room;
    connectDanmuServer();
}

void DouyuTcpSocket::onClose()
{
    qDebug()<<"onClose";
    close();
}

void DouyuTcpSocket::onTimerRec()
{
    if(!m_bAutoRecennect){
        return;
    }
    connectDanmuServer();
}

QString DouyuTcpSocket::STTSerialization(QStringList &key_list,QStringList &value_list)
{
    QStringList list;
    for(int i=0; i < key_list.count(); i++)
    {
        QString key = key_list.at(i);
        QString value = value_list.at(i);
        list<<QString("%1@=%2").arg(key.replace("@","@A").replace('/',"@S"))
                                    .arg(value.replace("@","@A").replace('/',"@S"));
    }
    return list.join('/');
}

QString DouyuTcpSocket::STTSerialization(QMap<QString, QString> &map)
{
    QStringList list;
    for(int i = 0;i < map.count();i++)
    {
        QString key = map.keys().at(i);
        QString value = map.values().at(i);
        list<<QString("%1@=%2").arg(key.replace("@","@A").replace('/',"@S"))
                                    .arg(value.replace("@","@A").replace('/',"@S"));
    }
    return list.join('/');
}

QMap<QString,QString> DouyuTcpSocket::STTDeserialization(QString &ser_str)
{
    QStringList list = ser_str.split('/');
    QMap<QString,QString> map;
    QString pattern = "([\\w\\W]+)@=([\\w\\W]*)";
    QRegExp regExp(pattern);
    regExp.setMinimal(false);
    for(int i = 0;i < list.count();i++)
    {
        if(regExp.indexIn(list.at(i)) != -1)
        {
            QString key = regExp.capturedTexts().at(1);
            key = key.replace("@S","/").replace("@A","@");
            QString value = regExp.capturedTexts().at(2);
            value = value.replace("@A","@").replace("@S","/");
            map.insert(key,value);
        }
    }
    return map;
}

qint32 DouyuTcpSocket::hexReverse_qint32(qint32 number)
{
    qint32 numTo = 0;
    qint8 _1,_2,_3,_4;
    _1 = number&0x000000ff;
    _2 = (number&0x0000ff00) >> 8;
    _3 = (number&0x00ff0000) >> 16;
    _4 = (number&0xff000000) >> 24;
    numTo = _1 << 24;
    numTo += _2 << 16;
    numTo += _3 << 8;
    numTo += _4 ;
    return numTo;
}


