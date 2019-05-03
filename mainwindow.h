#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QDate>
#include "networkaccess.h"
#include "jsonparse.h"
#include "douyutcpsocket.h"

class QTimer;
class QThread;
class QLabel;
class ChatMsg;
class singleCar;
class SqlMgr;
class SCarRankModel;
class QNetworkAccessManager;
class QNetworkReply;
namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void sigConnectDM(const QString &room);
    void sigCloseDM();
private slots:
    void htmlContent(const QString html);
    void showChatMessage(const QMap<QString, QString> &massage);
    void start();
    void onClear();
    void onClose();
    void onUpload(bool bAutoUpload = false);
    void onExpand();
    void onTrayClick(QSystemTrayIcon::ActivationReason reason);
    void onTrayMsgClick();
    void onCmbChange(int);
    void onUpdateRank();
    void postFinished();
    void onUpdateStat(const QString &str);
    void onLog(const QString &str);
    void onBtnPic();
    void onAutoUpload();
    void onCheckUploadTime();
protected:
    void closeEvent(QCloseEvent *event);
private:
    void uploadMsg(const ChatMsg* msg, const QDate &msgDate);
private:
    Ui::MainWindow *ui;
    NetworkAccess *network_access;
    DouyuTcpSocket *tcpSocket;
    bool m_bTray {false};
    QSystemTrayIcon *m_pTray{nullptr};
    QIcon m_iconMsg;
    QLabel *m_pLabStat{nullptr};
    QLabel *m_pLabTime{nullptr};
    singleCar *m_pSCar{nullptr};
    SqlMgr *m_pSqlMgr{nullptr};
    SCarRankModel *m_pRankModel{nullptr};
    QNetworkAccessManager *m_pNetMgr{nullptr};
    QThread *m_pThSocket{nullptr};
    QThread *m_pThSCar{nullptr};
    QTimer *m_pTimerUpdate{nullptr};
};

#endif // MAINWINDOW_H
