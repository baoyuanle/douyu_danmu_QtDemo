﻿#include "douyutcpsocket.h"
#include "danmuconfig.h"
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>
#include <QDataStream>


DouyuTcpSocket::DouyuTcpSocket(QObject *parent)
    :QObject(parent)
{

    this->danmu_rid = "";
    request_state = "";
    timer = new QTimer(this);
    connect(&tcpDanmuSoc,SIGNAL(connected()),this,SLOT(loginAuth()));
    connect(&tcpDanmuSoc,SIGNAL(readyRead()),this,SLOT(readDanmuMessage()));
    connect(&tcpDanmuSoc,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(displayError(QAbstractSocket::SocketError)));
    connect(&tcpDanmuSoc,SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this,SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(timer,SIGNAL(timeout()),this,SLOT(keepAlive()));


}

DouyuTcpSocket::~DouyuTcpSocket()
{
    tcpDanmuSoc.close();
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
    QByteArray inBlock = tcpDanmuSoc.readAll(); //接收数据块
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
        timer->start(_Douyu_DanmuServer_Intervals);
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
    auto iWrited = tcpDanmuSoc.write(outBlock);
    qDebug()<<"iWrited:"<<iWrited;
    qDebug()<<"iWrited:"<<outBlock;
    outBlock.clear();
    delete []pCont;
}

void DouyuTcpSocket::connectDanmuServer(QString &roomid)
{

    if(tcpDanmuSoc.state() == QAbstractSocket::ConnectedState)
    {
        tcpDanmuSoc.abort();
    }
    danmu_rid = roomid;
    for(int i=0;i<5;++i)
    {
        tcpDanmuSoc.connectToHost(_Douyu_DanmuServer_HostName,
                              _Douyu_DanmuServer_Port);
        if (tcpDanmuSoc.waitForConnected(2000))
              {
            qDebug("Connected!");
            break;
        }
        else {
            tcpDanmuSoc.abort();
        }
    }
}

void DouyuTcpSocket::close()
{
    if(tcpDanmuSoc.state() == QAbstractSocket::ConnectedState)
    {
        tcpDanmuSoc.abort();
    }
    timer->stop();
}

void DouyuTcpSocket::displayError(QAbstractSocket::SocketError error)
{

    QString error_str = tcpDanmuSoc.errorString();
    qDebug()<<error_str;
    tcpDanmuSoc.close();
}

void DouyuTcpSocket::keepAlive()
{
    timer->stop();
    if(request_state == "receiveDanmu")
    {
        QString tick = QString::number(QDateTime::currentMSecsSinceEpoch()/1000);
        QStringList key_list = (QStringList()
                                <<"type"
                                );
        QStringList value_list = (QStringList()
                                  <<"mrkl" //登录请求
                    );
        QString content = STTSerialization(key_list,value_list);
        this->messageWrite(content);
    }
    timer->start(_Douyu_DanmuServer_Intervals);
}

void DouyuTcpSocket::stateChanged(QAbstractSocket::SocketState state)
{
    qDebug()<<state;
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


