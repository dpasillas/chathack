#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QtNetwork>
#include "statustype.h"

enum ServerCommand{CLOGIN, CJOIN, CLEAVE, CLOGOUT, CEXIT, CULROOM, CSMROOM, CRECVMSG, CUUID, INCOMPLETE, INVALID, NUM_COMMANDS};

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = 0);
    ~Client();
signals:
  //  void ConnectionTimeout();
  //  void ServerResponseError();
    void loginSuccess(QString current_channel);
    void recievedText(QString channel, QString user, QString msg);
    void logout();
    void join(QString);
    void die();
    void leave(QString);
    void usersListRoom(QStringList);
    void userNameTaken(QString);
    void userListFailed();

   // void LoginFailure();
   // void ForcedDisconnect();
      //void LOGIN(QString n1, QString n2);
      //void LOGOUT();
      //void EXIT();
public slots:
    void handleConnection();
   // void toggleMic();
   // void SwapChannels(Qstring name);
    void ReadSocket();
    void handleError(QAbstractSocket::SocketError e);

    void slogin(QString userName, QString roomName);
    void slogout();
    void sexit();
    void sjoin(QString channel, QString type);
    void sleave(QString channel, QString type);
    void ssmroom(QString room, QString type, QString message);
    void sulroom(QString room);
    void suuid(int uuid);

protected:
    void loadServerConfig();

private:

    ServerCommand getMsgStatus(QString message);
    ServerCommand getCommand(QString command);
    void ParseMessage(QString &msg);

    void clogin();
    void clogout();
    void cjoin();
    void cexit();
    void cleave();
    void csmroom();
    void crecvmsg();
    void culroom();
    void cuuid();
    void invalidMessage();
    void unknownMessage();


    QTcpSocket *tcpSocket;
    bool micOn;
    bool exitFlag;
    quint16 blockSize;
    //std::vector<QString> cur_args;
    QStringList cur_args;
    QString messageSoFar;
    QString roomName, userName;
    QQueue<QString> messageBuffer;
    int uuid;

    friend class MainWindow;

    QString hostAddress;
    quint16 port;

};

#endif // CLIENT_H
