#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QSystemTrayIcon>
#include "networkaccess.h"
#include "jsonparse.h"
#include "douyutcpsocket.h"

class ChatMsg;
class singleCar;
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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void htmlContent(const QString html);
    void showChatMessage(QMap<QString, QString>);
    void showChatMessageString(QString message);
    void start();
    void onClear();
    void onClose();
    void onUpload();
    void onTrayClick(QSystemTrayIcon::ActivationReason reason);
    void onTrayMsgClick();
    void onCmbChange(int);
    void onUpdateRank();
    void postFinished();
protected:
    void closeEvent(QCloseEvent *event);
private:
    void uploadMsg(const ChatMsg* msg);
private:
    Ui::MainWindow *ui;
    NetworkAccess *network_access;
    DouyuTcpSocket *tcpSocket;
    bool m_bTray {false};
    QSystemTrayIcon *m_pTray{nullptr};
    QIcon m_iconMsg;
    singleCar *m_pSCar{nullptr};
    SCarRankModel *m_pRankModel{nullptr};
    QNetworkAccessManager *m_pNetMgr{nullptr};
};

#endif // MAINWINDOW_H
