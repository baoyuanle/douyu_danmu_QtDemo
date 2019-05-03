#ifndef DOUYUTCPSOCKET_H
#define DOUYUTCPSOCKET_H

#include <QtCore>
#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QCryptographicHash>
#include <QMap>
#include <QTimer>
/**
 * @brief The TcpSocket class
 * 连接弹幕服务器,负责与弹幕服务器进行通信
 */
class DouyuTcpSocket:public QObject
{
    Q_OBJECT
public:
    DouyuTcpSocket(QObject *parent = nullptr);
    ~DouyuTcpSocket();
    void setPort(unsigned short port);
signals:

    void chatMessage(const QMap<QString, QString> &massage);
    void chatMessageString(QString);
    void sigUpdateStat(const QString &str);

private:

    /**
     * @brief hexReverse_uint32
     * 32位整数大小端转换
     * @param number
     */
    qint32 hexReverse_qint32(qint32 number);

    /**
     * @brief messageWrite
     * 向服务器写消息
     * @param content
     */
    void messageWrite(QString &content);
private slots:
    /**
     * @brief loginAuth
     * 登录授权
     */
    void loginAuth();
    void readDanmuMessage();
    void displayError(QAbstractSocket::SocketError error);
    void keepAlive();//发送心跳包
    void stateChanged(QAbstractSocket::SocketState state);
    void onConnect(const QString &room);
    void onClose();
    void onTimerRec();
private:
    void connectDanmuServer(); //连接弹幕服务器信号槽
    void reconnect();
    void close();
    /**
     * @brief STTSerialization
     * STT序列化
     * @return
     */
    QString STTSerialization(QMap<QString,QString> &map);
    //保证按输入数组顺序发送,不知道是否有必要
    QString STTSerialization(QStringList &key_list, QStringList &value_list);
    /**
     * @brief STTDeserialization
     * STT反序列化
     * @param ser_str
     * @return
     */
    QMap<QString,QString> STTDeserialization(QString &ser_str);

    QString danmu_rid; //roomid
    QString request_state;
    bool m_bAutoRecennect{false};
    QTimer *timerHeart{nullptr};
    QTimer *timerReconnect{nullptr};
    QTcpSocket *pTcpDanmuSoc{nullptr};
    QByteArray outBlock;
    unsigned short m_uPort{0};
    unsigned short m_uConFailTime{0};
};

#endif // DOUYUTCPSOCKET_H
