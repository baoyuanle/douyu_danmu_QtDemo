#ifndef CONFIG_H
#define CONFIG_H
#include <QtCore>

/**
 * @brief _Douyu_Room_Pattern 正则匹配
 */
const QString _Douyu_Room_Pattern = "ROOM = (\\{[\\w\\W]+\\});";
const QString _Douyu_Room_Args_Pattern = "ROOM.args = (\\{[\\w\\W]+\\});";
const QString _Douyu_ServerConfig_Pattern = "\\{[\\w\\W]+\\}";
const QString _Douyu_MagicString = "7oE9nPEG9xXV69phU31FYCLUagKeYtsF";
const QString _Douyu_DanmuServer_HostName = "openbarrage.douyutv.com";
const int     _Douyu_DanmuServer_Port = 8601;
const int     _Douyu_DanmuServer_Intervals = 1000*30; //以毫秒记
const QString _Douyu_Room_gid = "-9999";
const qint32 _Douyu_CTS_Num = 0xb1020000; //斗鱼客户端到服务端的固定字段

const QString _YESAPISERVER_SECRECT ="改成自己的小白api secret";
const QString _YESAPISERVER_KEY = "小白api key";
/**
 * @brief _Douyu_RoomId json key集合
 */
const QStringList _Douyu_RoomId = (QStringList()<<"room_id");
const QStringList _Douyu_ServerConfig = (QStringList()<<"server_config");

struct ChatMsg
{
    unsigned int uid{0};
    QString nn;
    QString txt;
    unsigned int level{0};
    unsigned int nl{0};//贵族等级
    QString bnn;//牌子
    unsigned int bl{0};//牌子等级
    unsigned int count{0};//次数
    time_t tmRecent{0};
};
Q_DECLARE_METATYPE(ChatMsg);

#endif // CONFIG_H

