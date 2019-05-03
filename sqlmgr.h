#ifndef SQLMGR_H
#define SQLMGR_H
#include "danmuconfig.h"
#include <Qdate>

class SqlMgr
{
public:
    SqlMgr();
    ~SqlMgr();
    void begin();
    bool insertSCmsg(const ChatMsg& msg, const QDate &msgDate);
    void end();

private:
    void init();
};

#endif // SQLMGR_H
