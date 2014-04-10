#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "client.h"
#include <QHash>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void closeButton();
    void loginButton();
    void logoutButton();
    void joinButton();
    void leaveButton();
    void emailButton();
    void displayLoginError(QString);
    void login(QString room);
    void logout();
    void sendMessageButton();
    void handleSendMessage(QString msg);
    void handleRecieveText(QString channel, QString user, QString msg);
    void joinRoom(QString room);
    void leaveRoom(QString room);
    void usersListRoom(QStringList users);
    void userNameTaken(QString user);
    void userListFailed();
    void loadServerConfig();
    void changeServerConfig();

signals:
    void tryClose();
    void tryLogout();
    void tryJoin(QString,QString);
    void tryLeave(QString,QString);
    void toggleMic();
    void badLogin(QString);
    void tryLogin(QString, QString);
    void trySendMessage(QString,QString,QString);
    void requestUsersList(QString);
    void startConnection(QString, QString);

    //zach's signals
//    void ConnectionTimeout();
//    void ServerResponseError();
//    void LoginSuccess(QString current_channel);
//    void LoginFailure();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent *e);
    bool eventFilter(QObject *o, QEvent *e);

private:
    void showLoggedInStuff();
    void hideLoggedInStuff();
    void setRoom(QString room);

    Ui::MainWindow *ui;
    int mouseClickX, mouseClickY;
    int loginErrorTimer;
    bool movable;

    Client *c;
    QThread *workerThread;
    QHash<QString, QStringList> users;
    QHash<QString, QString> roomText;

    QAction *settingsAction;
};

#endif // MAINWINDOW_H
