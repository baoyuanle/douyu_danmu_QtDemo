#include "sqlmgr.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

const QString g_DbConnectName = "PiCarSoSCmsg";
const QString g_DatabaseName = "PiCarSo.db";
const QString g_TableName = "DailyMsg";

SqlMgr::SqlMgr()
{
    init();
}

SqlMgr::~SqlMgr()
{
    auto db = QSqlDatabase::database(g_DbConnectName);
    db.close();
}

void SqlMgr::init()
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", g_DbConnectName);
    database.setDatabaseName(g_DatabaseName);
    if (!database.open()){
        qDebug() << "Error: Failed to connect database." << database.lastError();
        return;
    }else{
        qDebug() << "Succeed to connect database." ;
    }

    auto lsTables = database.tables();
    bool bExist = false;
    for(auto &table : lsTables){
        if(g_TableName == table){
            bExist = true;
            break;
        }
    }

    if(!bExist){//创建表格
        QString strSql = QString("CREATE TABLE %1("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "nn TEXT, "
                                 "txt TEXT, "
                                 "count INT NOT NULL DEFAULT 1, "
                                 "dm_time TEXT NOT NULL, "//统计时间
                                 "last_time TEXT NOT NULL, "//最后一条独轮车时间
                                 "lev INT NOT NULL DEFAULT 1, "
                                 "bnn TEXT, "
                                 "bl INT DEFAULT 0)").arg(g_TableName);
        QSqlQuery sql_query(database);
        if(!sql_query.exec(strSql))
        {
            qDebug() << "Error: Fail to create table."<< sql_query.lastError();
        }else{
            qDebug() << "Table created!";
        }
    }else{
        qDebug()<<"Table exist.";
    }
}

void SqlMgr::begin()
{
    auto db = QSqlDatabase::database(g_DbConnectName);
    db.transaction();
}

bool SqlMgr::insertSCmsg(const ChatMsg &msg, const QDate &msgDate)
{
    auto db = QSqlDatabase::database(g_DbConnectName);
    QSqlQuery sql_query(db);
    auto strTime = msgDate.toString("yyyy-MM-dd 00:00:00");
    auto strLastTime = QDateTime::fromTime_t(msg.tmRecent).toString("yyyy-MM-dd hh:mm:ss");
    QString strSql = QString("INSERT INTO %1("
                             "id, nn, txt, count, dm_time, last_time, lev, bnn, bl) "
                             "VALUES ("
                             "NULL, '%2', '%3', %4, '%5', '%6', %7, '%8', %9)"
                             ).arg(g_TableName).arg(msg.nn).arg(msg.txt).
                            arg(msg.count).arg(strTime).arg(strLastTime).arg(msg.level).
                            arg(msg.bnn).arg(msg.bl);
    if(!sql_query.exec(strSql))
    {
        qDebug() << "Error: Fail to insert."<< sql_query.lastError();
        return false;
    }else{
        qDebug() << "inserted! msg nn:"<<msg.nn;
    }
    return true;
}

void SqlMgr::end()
{
    auto db = QSqlDatabase::database(g_DbConnectName);
    db.commit();
}
