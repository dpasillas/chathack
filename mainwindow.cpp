#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QMouseEvent>
#include <QDebug>
#include <QScrollBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QHostInfo>
#include "program.cc"
#include "serversettingsdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    loginErrorTimer(0), movable(false),
    c(new Client()),
    workerThread(new QThread(this)),
    settingsAction(new QAction(this))
{
    workerThread->start();
    connect(workerThread,SIGNAL(finished()),workerThread,SLOT(deleteLater()));
    ui->setupUi(this);
    ui->menuBar->hide();
    ui->mainToolBar->hide();
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    hideLoggedInStuff();
    //ui->textBrowser->setOpenExternalLinks(true);

    //gui actions
    settingsAction->setShortcut(QKeySequence("Alt+S"));
    ui->serverConfigButton->addAction(settingsAction);
    connect(settingsAction,SIGNAL(triggered()),this,SLOT(changeServerConfig()));

    //button slots
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(closeButton()));
    connect(ui->logoutButton,SIGNAL(clicked()),this,SLOT(logoutButton()));
    connect(ui->toggleMicButton,SIGNAL(clicked()),this,SIGNAL(toggleMic()));
    connect(ui->joinButton,SIGNAL(clicked()),this,SLOT(joinButton()));
    connect(ui->leaveButton,SIGNAL(clicked()),this,SLOT(leaveButton()));
    connect(ui->emailButton,SIGNAL(clicked()),this,SLOT(emailButton()));
    connect(ui->serverConfigButton,SIGNAL(clicked()),settingsAction,SLOT(trigger()));

    //GUI login slots
    connect(ui->loginButton,SIGNAL(clicked()),this,SLOT(loginButton()));
    connect(ui->roomNameLine,SIGNAL(returnPressed()),this,SLOT(loginButton()));
    connect(ui->usernameLine,SIGNAL(returnPressed()),this,SLOT(loginButton()));

    //send chat messages to client
    connect(ui->sendButton,SIGNAL(clicked()),this,SLOT(sendMessageButton()));
    connect(ui->chatBox,SIGNAL(returnPressed()),this,SLOT(sendMessageButton()));

    //send other commands to client
    connect(this,SIGNAL(tryLogin(QString,QString)),c,SLOT(slogin(QString,QString)));
    connect(this,SIGNAL(tryLogout()),c,SLOT(slogout()));
    connect(this,SIGNAL(trySendMessage(QString,QString,QString)),c,SLOT(ssmroom(QString,QString,QString)));
    connect(this,SIGNAL(requestUsersList(QString)),c,SLOT(sulroom(QString)));
    connect(this,SIGNAL(tryJoin(QString,QString)),c,SLOT(sjoin(QString,QString)));
    connect(this,SIGNAL(tryLeave(QString,QString)),c,SLOT(sleave(QString,QString)));
    //connect(this,SIGNAL(trySendMessage(QString)),this,SLOT(handleSendMessage(QString)));

    //recieve messages from clients and respond to them
    connect(c,SIGNAL(loginSuccess(QString)),this,SLOT(login(QString)));
    connect(c,SIGNAL(logout()),this,SLOT(logout()));
    connect(c,SIGNAL(recievedText(QString,QString,QString)),this,SLOT(handleRecieveText(QString,QString,QString)));
    connect(c,SIGNAL(join(QString)),this,SLOT(joinRoom(QString)));
    connect(c,SIGNAL(die()),this,SLOT(close()));
    connect(c,SIGNAL(leave(QString)),this,SLOT(leaveRoom(QString)));
    connect(c,SIGNAL(usersListRoom(QStringList)),this,SLOT(usersListRoom(QStringList)));
    connect(c,SIGNAL(userNameTaken(QString)),this,SLOT(userNameTaken(QString)));
    connect(c,SIGNAL(userListFailed()),this,SLOT(userListFailed()));

    connect(this,SIGNAL(badLogin(QString)),this,SLOT(displayLoginError(QString)));
    c->moveToThread(workerThread);

    loadServerConfig();

    ui->roomsListWidget->installEventFilter(this);
    ui->usersListWidget->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    c->deleteLater();
    workerThread->quit();

    workerThread->wait();
}

void MainWindow::closeButton()
{
    emit tryClose();
    close();
}

void MainWindow::loginButton()
{
    static QRegExp validName("([0-9a-zA-Z]){1,30}");
    QString roomName(ui->roomNameLine->text()), username(ui->usernameLine->text());
    if(!validName.exactMatch(roomName))
    {
        emit badLogin("Illegal Room Name!");
        return;
    }
    if(!validName.exactMatch(username))
    {
        emit badLogin("Illegal Username!");
        return;
    }

    emit tryLogin(username, roomName);
    //login();
}

void MainWindow::logoutButton()
{
    emit tryLogout();
    logout();
}

void MainWindow::joinButton()
{
    static QRegExp validName("([0-9a-zA-Z])+");
    QString room = QInputDialog::getText(this,"RoomName","Room:");
    if(!validName.exactMatch(room))
        return;
    emit tryJoin(room,"0");
}

void MainWindow::leaveButton()
{
    QList<QListWidgetItem*> selected = ui->roomsListWidget->selectedItems();
    if(selected.size())
        emit tryLeave(selected[0]->text(),"0");
}

void MainWindow::emailButton()
{
    QString d = QInputDialog::getText(this,"Email","Please enter your email:");
    if(d.isEmpty())
        return;
    QRegExp re;

    string destination = d.toStdString(); // Format: Name <example@something.com>
    string subject = QString("Conversation @ %1").arg(c->roomName).toStdString();     // Format: Subject
    string source = "Bat Chat <bat@chat.com>";      // Format: My Name <example@another.com>
    string text = this->roomText[c->roomName].replace("color:#ffffff","color:#555555").toStdString();        // Rest of the text
    /*destination = "Sam <shong010@ucr.edu>";
    subject = "Hello World!!";
    source = "torch <torch@stuff.com>";
    text = "sam: lol!\njohn: stuff\nsam: again";*/
    qDebug() << "Send Grid! :)";
    sendGrid(destination, subject, source, text);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    mouseClickX = e->x();
    mouseClickY = e->y();

    movable = true;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
   Q_UNUSED(e)
    movable = false;
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(movable)
        move(e->globalX()-mouseClickX,e->globalY()-mouseClickY);

}

void MainWindow::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == loginErrorTimer)
    {
        e->accept();
        ui->loginErrorLabel->setText("");
    }

}

bool MainWindow::eventFilter(QObject *o, QEvent *e)
{
    if(o == ui->roomsListWidget || o == ui->usersListWidget)
    {
        if(e->type() == QEvent::MouseButtonPress)
            return true;
    }
    return QObject::eventFilter(o,e);

}

void MainWindow::showLoggedInStuff()
{
    ui->logoutButton->show();
    ui->toggleMicButton->show();
    ui->chatIcon->show();
    ui->roomLabel->show();
    ui->chatMessageLabel->show();
    ui->leaveButton->show();
    ui->joinButton->show();
    ui->emailButton->show();
    ui->serverConfigButton->hide();
}

void MainWindow::hideLoggedInStuff()
{
    ui->logoutButton->hide();
    ui->toggleMicButton->hide();
    ui->chatIcon->hide();
    ui->roomLabel->hide();
    ui->chatMessageLabel->hide();
    ui->leaveButton->hide();
    ui->joinButton->hide();
    ui->emailButton->hide();
    ui->serverConfigButton->show();
}

void MainWindow::setRoom(QString room)
{
    QString user = c->userName;
    ui->roomLabel->setText(QString("%1@%2").arg(user,room));
    ui->usersListWidget->clear();
    ui->usersListWidget->addItems(this->users[room]);
    ui->textBrowser->setHtml(this->roomText[room]);
    foreach(QListWidgetItem * item, ui->roomsListWidget->selectedItems())
    {
        item->setSelected(false);
    }

    foreach(QListWidgetItem * item, ui->roomsListWidget->findItems(room,Qt::MatchExactly))
    {
        item->setSelected(true);
    }

    ui->textBrowser->setHtml(this->roomText[room]);
    c->roomName = room;

}

void MainWindow::displayLoginError(QString msg)
{
    ui->loginErrorLabel->setText(msg);
    if(loginErrorTimer){
        killTimer(loginErrorTimer);
        loginErrorTimer = 0;
    }
    loginErrorTimer = startTimer(2000);

}

void MainWindow::login(QString room)
{
    //ui->usernameLine->clear();
    ui->roomNameLine->clear();
    showLoggedInStuff();
    ui->stackedWidget->setCurrentWidget(ui->chatPage);
    ui->roomsListWidget->clear();
    ui->roomsListWidget->addItem(room);
    setRoom(room);
}

void MainWindow::logout()
{
    hideLoggedInStuff();
    ui->roomNameLine->setFocus();
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

void MainWindow::sendMessageButton()
{
    emit trySendMessage(c->roomName,"0",ui->chatBox->text().toHtmlEscaped());
    ui->chatBox->clear();
}

void MainWindow::handleSendMessage(QString msg)
{
    //qDebug() << msg;
    handleRecieveText("1","dpasillas",msg);
}

void MainWindow::handleRecieveText(QString channel, QString user, QString msg)
{
    QString messageFormat(
                "<p "
                "style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
                "<span style=\" font-weight:600; color:#36e1d3;\">"
                "%1: "
                "</span>"
                "<span style=\" color:#ffffff\">"
                "%2"
                "</span></p>"
                );
    this->roomText[channel].append(messageFormat.arg(user,msg));
    //ui->textBrowser->append(messageFormat.arg(user,msg));
    //qDebug() << QString("HTML escaped: ") + messageFormat.arg(user,msg);

    if(channel == c->roomName)
        ui->textBrowser->setHtml(this->roomText[channel]);

    ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
}

void MainWindow::joinRoom(QString room)
{
    ui->roomsListWidget->addItem(room);
    setRoom(room);
}

void MainWindow::leaveRoom(QString room)
{
    this->roomText.remove(room);
    this->users.remove(room);

    ui->roomsListWidget->clear();
    ui->roomsListWidget->addItems(users.keys());

    if(ui->roomsListWidget->count() > 0)
    {
        setRoom(ui->roomsListWidget->item(0)->text());
    }
    else
    {
        emit tryLogout();
        logout();
    }
}

void MainWindow::usersListRoom(QStringList users)
{
    qDebug() << "void MainWindow::usersListRoom(QStringList users)";
    this->users[c->roomName] = users;
    setRoom(c->roomName);
}

void MainWindow::userNameTaken(QString user)
{
    QMessageBox::warning(this,"User Exists!", QString("User \"%1\" already exists!").arg(user));
}

void MainWindow::userListFailed()
{
    QMessageBox::warning(this,"No User List","Retrieving the user list for this room failed!");
}

void MainWindow::loadServerConfig()
{
    QSettings settings;
    QString ha = settings.value("server/hostaddress","").toString();
    QHostInfo info = QHostInfo::fromName(ha);
    if(info.addresses().size())
        c->hostAddress = info.addresses().front().toString();
    else
        c->hostAddress = "127.0.0.1";

    //settings.setValue("server/hostaddress");

    //c->hostAddress = (ha=="")?QString("192.168.1.135"):ha;
    quint16 p = settings.value("server/port",0).toUInt();
    c->port = (p==0)?quint16(6501):p;

}

void MainWindow::changeServerConfig()
{
    QSettings settings;
    QString hostName = settings.value("server/hostname","").toString();

    if(hostName == "")
        hostName = "localhost";

    ServerSettingsDialog *dialog = new ServerSettingsDialog(this,hostName,QString::number(c->port));
    int status = dialog->exec();

    if(status == QDialog::Accepted)
    {
        QHostInfo info = QHostInfo::fromName(dialog->hostName);
        if(info.addresses().size())
        {
            c->hostAddress = info.addresses().front().toString();
            qDebug() << info.addresses().size();
            c->port = dialog->port.toInt();

            //settings.setValue("server/hostaddress",c->hostAddress);
            settings.setValue("server/hostname",dialog->hostName);
            settings.setValue("server/port",c->port);
        }
        else
        {
            QMessageBox::warning(this,"Host Resolution Error", "Unable to resolve the address of the specified host, please try again!");
        }
    }
    else
        qDebug() << "Failure!";
    qDebug() <<"Finished! :P";

    delete dialog;

}
