#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stringgenerator.h"
#include "danmuconfig.h"
#include <QRegExp>
#include <QSystemTrayIcon>
#include "singlecar.h"
#include "scarrankmodel.h"
#include <QNetworkAccessManager>
#include <QMap>
#include <QCryptographicHash>
#include <QDateTime>
#include <QStatusBar>
#include <QLabel>
#include <QThread>

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_bTray = QSystemTrayIcon::isSystemTrayAvailable();

    if(m_bTray){
        m_bTray = QSystemTrayIcon::supportsMessages();
    }
    m_bTray =false;
    m_iconMsg =QIcon(":/res/bad.png");
    ui->setupUi(this);
    ui->plainTextEdit->setMaximumBlockCount(10000);
    ui->lineEdit_roomid->setText("562590");
    ui->cmbRoom->addItem("562590","562590");
    ui->cmbRoom->addItem("101","101");
    ui->cmbRoom->addItem("5377067","5377067");
    ui->cmbRoom->addItem("99999","99999");
    ui->cmbRoom->addItem("156277","156277");
    ui->cmbRoom->addItem("610588","610588");

    //状态栏
    QStatusBar * sBar = statusBar();//创建状态栏
    m_pLabStat = new QLabel(this);
    m_pLabStat->setText(tr("未连接"));
    sBar->addWidget(m_pLabStat);//状态栏添加组件

    m_pRankModel = new SCarRankModel();
    ui->tableRank->setModel(m_pRankModel);
    ui->tableRank->setColumnWidth(1,300);
    network_access = new NetworkAccess();
    tcpSocket = new DouyuTcpSocket();
    m_pThSocket = new QThread();
    tcpSocket->moveToThread(m_pThSocket);
    m_pThSocket->start();
    m_pSCar = new singleCar();
    m_pThSCar = new QThread();
    m_pSCar->moveToThread(m_pThSCar);
    m_pThSCar->start();
    connect(ui->pushButton,SIGNAL(clicked(bool)),this,SLOT(start()));
    connect(ui->btnClear,SIGNAL(clicked(bool)),this,SLOT(onClear()));
    connect(ui->btnStop,SIGNAL(clicked(bool)),this,SLOT(onClose()));
    connect(ui->btnUpload,SIGNAL(clicked(bool)),this,SLOT(onUpload()));
    connect(ui->btnExpand,SIGNAL(clicked(bool)),this,SLOT(onExpand()));
    connect(ui->cmbRoom, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbChange(int)));
    connect(network_access,SIGNAL(pageLoadFinished(QString)),
            this,SLOT(htmlContent(QString)));
    connect(this, SIGNAL(sigConnectDM(const QString&)), tcpSocket, SLOT(onConnect(const QString&)));
    connect(this, SIGNAL(sigCloseDM()), tcpSocket, SLOT(onClose()));
    connect(tcpSocket,SIGNAL(chatMessage(QMap<QString,QString>)),this,SLOT(showChatMessage(QMap<QString,QString>)));
    connect(tcpSocket,SIGNAL(chatMessage(QMap<QString,QString>)),m_pSCar,SLOT(onNewMsg(QMap<QString,QString>)));
    connect(tcpSocket,SIGNAL(sigUpdateStat(const QString &)), this, SLOT(onUpdateStat(const QString &)));
    connect(m_pSCar,SIGNAL(sigUpdateRank()),this,SLOT(onUpdateRank()));
    if(m_bTray)
    {
       auto minimizeAction = new QAction(tr("close"), this);
        connect(minimizeAction, &QAction::triggered, this, &QWidget::close);

        auto pMenu = new QMenu(this);
        pMenu->addAction(minimizeAction);
        m_pTray = new QSystemTrayIcon(this);
        m_pTray->setIcon(m_iconMsg);
        m_pTray->setContextMenu(pMenu);
        m_pTray->setToolTip("DmPro");
        connect(m_pTray, &QSystemTrayIcon::messageClicked, this, &MainWindow::onTrayMsgClick);
        connect(m_pTray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayClick);
    }

}

MainWindow::~MainWindow()
{
    m_pThSocket->exit();
    m_pThSocket->deleteLater();
    m_pThSCar->exit();
    m_pThSCar->deleteLater();
    delete m_pSCar;
    delete tcpSocket;
    delete network_access;
    delete ui;
}

void MainWindow::start()
{
    QString roomid = (ui->lineEdit_roomid->text()).trimmed();
    QRegExp rx("[0-9a-zA-Z]+");
    rx.setMinimal(false);
    if(rx.exactMatch(roomid))
    {
        bool ok = false;
        roomid.toInt(&ok);
        if(!ok)
        {
            QString url_str = QString("http://www.douyu.com/%1").arg(roomid);
            QUrl url = QUrl(QString(url_str));
            network_access->loadingPage(url);
        }
        else
        {
            qDebug()<<"connectting,roomid:"<<roomid;
            emit sigConnectDM(roomid);
        }
    }
    else
    {
        ui->lineEdit_roomid->setText(QString(""));
    }
}

void MainWindow::onClear()
{
    ui->plainTextEdit->clear();
}

void MainWindow::onClose()
{
    emit sigCloseDM();
}

namespace  {
    const static QString c_url = "http://改成你的.api.yesapi.cn/";
    QString mkUrl(const QMap<QString, QString> &hash)
    {
        QString str = "?";
        for(auto ha = hash.cbegin();ha!=hash.cend();++ha){
            str+="&"+ha.key()+"="+ha.value();
        }
        return c_url + str;
    };
    QString md5(const QString& str){
        QByteArray byteText = str.toUtf8();
        QString strMd5 = QCryptographicHash::hash(byteText, QCryptographicHash::Md5).toHex();
        return  strMd5;
    }
    QString mkYesSIGN(const QMap<QString, QString> &hash){
        QString str;
        for(auto ha = hash.cbegin();ha!=hash.cend();++ha){
            str+=ha.value();
        }
        str+=_YESAPISERVER_SECRECT;
        str = md5(str);
        str = str.toUpper();
        qDebug() << "mkYesSIGN:"<<str;
        return str;
    }
    QString mkCreateSQL(const ChatMsg&msg){
        QString str="{";
        QMap<QString,QString> hs;
        hs["dm_time"]=QDateTime::currentDateTime().toString("yyyy-MM-dd 00:00:00");
        hs["nn"]=msg.nn;
        hs["lev"]=QString::number(msg.level);
        hs["txt"]=msg.txt;
        hs["count"]=QString::number(msg.count);
        hs["bnn"]=msg.bnn;
        hs["bl"]=QString::number(msg.bl);
        for(auto ha = hs.cbegin();ha!=hs.cend();++ha){
            str+="\""+ha.key()+"\""+":\""+ha.value()+"\",";
        }
        str.remove(str.length()-1,1);//去掉最后一个,
        str += "}";
        qDebug() << "mkCreateSQL:"<<str;
        return str;
    }
}
void MainWindow::onUpload()
{
    if(nullptr == m_pNetMgr){
        m_pNetMgr = new QNetworkAccessManager();
    }
    auto count = m_pRankModel->rowCount();
    for (int i=0; i<count; ++i) {
        auto vMsg = m_pRankModel->data(m_pRankModel->index(i,0),Qt::UserRole+1);
        ChatMsg msg;
        if(vMsg.canConvert<ChatMsg>()){
            msg = vMsg.value<ChatMsg>();
            uploadMsg(&msg);
        }
    }
}

void MainWindow::onExpand()
{
    if("both"==ui->btnExpand->text()){
        ui->btnExpand->setText("txt");
        ui->plainTextLog->hide();
    }else if ("txt"==ui->btnExpand->text()) {
        ui->btnExpand->setText("log");
        ui->plainTextEdit->hide();
        ui->plainTextLog->show();
    }else if ("log"==ui->btnExpand->text()) {
        ui->btnExpand->setText("both");
        ui->plainTextEdit->show();
        ui->plainTextLog->show();
    }
}

void MainWindow::uploadMsg(const ChatMsg* msg){
    if(nullptr == msg){
        return;
    }
    QNetworkRequest request;
    QMap<QString, QString> hsReq;
    hsReq["s"] = "App.Table.Create";
    hsReq["model_name"] = "dy562590";
    hsReq["data"] = mkCreateSQL(*msg);
    hsReq["app_key"] = _YESAPISERVER_KEY;
    QString strSign = mkYesSIGN(hsReq);
    hsReq["sign"] = strSign;
    auto url = mkUrl(hsReq);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    request.setUrl(QUrl(url));//setEncodedUrl
    QByteArray data;
    auto pReply = m_pNetMgr->post(request, data);
    qDebug() << "start post:"<<url;
    connect(pReply, &QNetworkReply::finished, this, &MainWindow::postFinished);
}

void MainWindow::onTrayClick(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
       {
            show();
            raise();
            if(m_pTray){
            m_pTray->hide();
            }
       }
       break;
    default:
        ;
    }
}

void MainWindow::onTrayMsgClick()
{
    show();
    raise();
    if(m_pTray){
    m_pTray->hide();
    }
}

void MainWindow::onCmbChange(int)
{
   auto str = ui->cmbRoom->currentData().toString();
   ui->lineEdit_roomid->setText(str);
}

void MainWindow::onUpdateRank()
{
    QList<ChatMsg> lsRank = m_pSCar->getRank();
    m_pRankModel->removeRows(0,m_pRankModel->rowCount());
    for(auto &rank:lsRank){
        m_pRankModel->add(rank);
    }
}

void MainWindow::postFinished()
{
    auto pReply = qobject_cast<QNetworkReply*>(sender());
    if(nullptr == pReply){
        return;
    }
    QByteArray bytes = pReply->readAll();
    QString strReply = bytes;
    qDebug()<<"reply:"<<strReply;
    const QVariant redirectionTarget = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {//如果网址跳转重新请求
        qDebug()<<"redirectedUrl:"<<redirectionTarget.toUrl();
    }
    pReply->deleteLater();
    pReply = nullptr;
}

void MainWindow::onUpdateStat(const QString &str)
{
    m_pLabStat->setText(str);
    onLog(str);
}

void MainWindow::onLog(const QString &str)
{
    QDateTime tm = QDateTime::currentDateTime();
    ui->plainTextLog->appendHtml(tm.toString("MM-dd hh:mm:ss ")+str);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit sigCloseDM();
    if(m_pTray)
    {
        if(m_pTray->isVisible()){
            event->accept();
        }else{
            m_pTray->show();
            hide();
            event->ignore();
        }
        return;
    }
    event->accept();
}


void MainWindow::htmlContent(const QString html)
{
    //正则数据提取JSON
    QString pattern = _Douyu_Room_Pattern;
    QRegExp regExp(pattern);
    regExp.setMinimal(true);
    QString json;
    int pos = 0;
    while((pos = regExp.indexIn(html,pos)) != -1)
    {
        json = regExp.capturedTexts().at(1);
        pos += regExp.matchedLength();
    }
    JSONParse parse;
    if(parse.init(json))
    {
        QString roomid = parse.getJsonValue(_Douyu_RoomId);
        emit sigConnectDM(roomid);
    }
    else
    {
        ui->lineEdit_roomid->setText(QString(""));
        ui->lineEdit_roomid->setPlaceholderText("加载失败");
        ui->plainTextEdit->setPlainText(QString(""));
    }

}

void MainWindow::showChatMessage(const QMap<QString, QString> &messageMap)
{
    /*QString nickname = messageMap["nn"];
    QString level = messageMap["level"];
    QString txt = messageMap["txt"];
    QString message = QString("<font style=\"color:#3B91C5;font-family:Microsoft YaHei\">%1</font> <font style=\"color:#E34945;font-family:consolas\">[lv.%2]</font><font style=\"color:#3B91C5;font-family:Microsoft YaHei\">:</font> <font style=\"color:#454545;font-family:Microsoft YaHei\">%3</font>").arg(nickname).arg(level).arg(txt);
    */
    auto strMsg = StringGenerator::getString(messageMap);
    if(!strMsg.isEmpty())
        ui->plainTextEdit->appendHtml(strMsg);

    if(m_bTray)
    {
        //m_pTray->showMessage(messageMap["nn"], messageMap["txt"], m_iconMsg, 10);
       // qDebug()<<messageMap["nn"]<<messageMap["txt"]<<"sgggg";
    }

}
