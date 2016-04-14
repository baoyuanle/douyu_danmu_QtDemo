﻿#include "douyutcpsocket.h"
#include "danmuconfig.h"
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>
#include <QDataStream>


DouyuTcpSocket::DouyuTcpSocket(QObject *parent)
    :QObject(parent)
{

    this->danmu_gid = "-9999";
    this->danmu_rid = "335166";
    request_state = "";
    timer = new QTimer(this);
    connect(&tcpDanmuSoc,SIGNAL(connected()),this,SLOT(loginAuth()));
    connect(&tcpDanmuSoc,SIGNAL(readyRead()),this,SLOT(readDanmuMessage()));
    connect(&tcpDanmuSoc,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(displayError(QAbstractSocket::SocketError)));
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
                            <<"username"
                            <<"password"
                            <<"roomid"
                            <<"rid"
                            <<"gid"
                            );
    QStringList value_list = (QStringList()
                              <<"loginreq" //登录请求
                              <<""
                              <<""
                              <<"335166"
                              <<danmu_rid //房间号
                              <<"-9999"
                );
    QString content = STTSerialization(key_list,value_list);
    const char *content_ptr = content.toStdString().c_str();
    QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
    qint32 length = 4 + 4 + content.length() + 1;// 2个uint32字段长度+内容长度+'\0'
    sendOut<<qint32(hexReverse_qint32(length))<<qint32(hexReverse_qint32(length))<<qint32(_Douyu_CTS_Num);
    outBlock.append(content_ptr);
    outBlock.append('\0');
    tcpDanmuSoc.write(outBlock);
    outBlock.resize(0);
    delete content_ptr;
    request_state = "loginReq";
}

void DouyuTcpSocket::readDanmuMessage()
{
    QByteArray inBlock = tcpDanmuSoc.readAll(); //接收数据块
    QString content;
    qint32 length;

    int pos = 0;
    do
    {
        length =  (uchar)inBlock.at(0 + pos);
        length += ((uchar)(inBlock.at(1 + pos)))<<8;
        length += ((uchar)(inBlock.at(2 + pos)))<<16;
        length += ((uchar)(inBlock.at(3 + pos)))<<24;
        content = inBlock.mid(pos+12,length);
        QMap<QString,QString> messageMap = STTDeserialization(content);



        //弹幕类型分析
        QString key = "cur_lev"; //出现表示服务端消息已经发完，可进入下一个步骤
        if(messageMap.keys().indexOf(key) != -1)
        {
            request_state = "joingroup";
        }
        key = "txt";
        if(messageMap.keys().indexOf(key) != -1)
        {
            emit chatMessage(messageMap);
        }
        pos = pos+length-9+13;
    }while(pos < inBlock.length());

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
                                  <<"-9999"
                    );
        QString content = STTSerialization(key_list,value_list);
        const char *content_ptr = content.toStdString().c_str();

        QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
        qint32 length = 4 + 4 + content.length() + 1;// 2个uint32字段长度+内容长度+'\0'
        sendOut<<qint32(hexReverse_qint32(length))<<qint32(hexReverse_qint32(length))<<qint32(_Douyu_CTS_Num);
        outBlock.append(content_ptr);
        outBlock.append('\0');
        tcpDanmuSoc.write(outBlock);
        outBlock.resize(0);
        request_state = "receiveDanmu";
        timer->start(1000*30);
        delete content_ptr;
    }
}

void DouyuTcpSocket::connectDanmuServer(QString &roomid)
{

    danmu_rid = roomid;
    QString hostName="openbarrage.douyutv.com";
    tcpDanmuSoc.connectToHost(hostName,8601);

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
                                <<"tick"

                                );
        QStringList value_list = (QStringList()
                                  <<"keeplive" //登录请求
                                  <<tick //时间戳
                    );
        QString content = STTSerialization(key_list,value_list);
        const char *content_ptr = content.toStdString().c_str();

        QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
        qint32 length = 4 + 4 + content.length() + 1;// 2个uint32字段长度+内容长度+'\0'
        sendOut<<qint32(hexReverse_qint32(length))<<qint32(hexReverse_qint32(length))<<qint32(_Douyu_CTS_Num);
        outBlock.append(content_ptr);
        outBlock.append('\0');
        tcpDanmuSoc.write(outBlock);
        outBlock.resize(0);
        delete content_ptr;
    }
    timer->start(1000*30);
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

