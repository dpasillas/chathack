#include "client.h"

#include <vector>
#include <string>
#include <iostream>
#include <QDebug>
#include <QWidget>
#include <QApplication>

//default settings moved to void loadServerConfig()

const quint16 BLOCK_SIZE = 1;
int NUM_ARGS[NUM_COMMANDS];

Client::Client(QObject *parent) :
    QObject(parent), micOn(false),exitFlag(false), uuid(-1)
{
    loadServerConfig();
    NUM_ARGS[CLOGIN] = 5;
    NUM_ARGS[CJOIN] = 4;
    NUM_ARGS[CLEAVE] = 4;
    NUM_ARGS[CLOGOUT] = 3;
    NUM_ARGS[CEXIT] = 3;
    NUM_ARGS[CULROOM] = 3;
    NUM_ARGS[CSMROOM] = 4;
    NUM_ARGS[CRECVMSG] = 5;
    NUM_ARGS[INVALID] = -1;
    NUM_ARGS[INCOMPLETE] = -1;
    NUM_ARGS[CUUID] = 3;

    tcpSocket = new QTcpSocket(this);
    blockSize = BLOCK_SIZE;
    //connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(exit()));
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(ReadSocket()));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(handleConnection()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(handleError(QAbstractSocket::SocketError)));

}

Client::~Client()
{
    tcpSocket->deleteLater();
    exitFlag = true;
}

void Client::handleConnection()
{
    qDebug() << "HANDLING CONNECTION!";
//    QString msg("slogin|" + roomName+ '|' + userName + "|slogin\n");
//    qDebug() << msg << "";
//    tcpSocket -> write(msg.toStdString().c_str());

    QString serverMessage("slogin|%1|%2|0|slogin\n");
    tcpSocket->write(serverMessage.arg(roomName,userName).toStdString().c_str());
}


void Client::slogin(QString userName, QString roomName)
{
    qDebug() << "STARTING LOGIN!";

    this->roomName = roomName;
    this->userName = userName;
    if(tcpSocket->state() != QAbstractSocket::ConnectedState){
        qDebug() << "Connecting to server...";
        tcpSocket -> connectToHost(hostAddress, port);
    }
    else
    {
        handleConnection();
    }

//    QString msg("slogin|" + Rname+ '|' + Cname + "|slogin\n");
//    qDebug() << msg << "";
//    int test = tcpSocket -> write(msg.toStdString().c_str());

//    QString recieved = "";
//    qDebug() << "test = " << test << "Entering wait loop" << "";
//    while(!hasCompleteMsg(recieved))
//    {
//        QApplication::processEvents();
//        sleep(1);
//        if(exitFlag)
//        {
//            qDebug() << "Exiting from login";
//            return;
//        }
//        //qDebug() << "POOOP!";

//        //ReadSocket();
//    }
//    qDebug() << "Exiting wait loop" << "";
//    if(recieved != "clogin")
//    {
//        qDebug() << "Server Response Error" << "";
//        //emit ServerResponseError(); //Will probably never happen Server should respond correctly...
//        cur_args.clear();
//        return;
//    }
//    std::vector<QString> recieved_args;
//    for (int i = 1; i < cur_args.size(); ++i)
//    {
//        if(cur_args[i] != "clogin")
//        {
//               recieved_args.push_back(cur_args[i]);
//        }
//        else
//        {
//            break;
//        }
//    }
//    if(recieved_args.size() != 3)
//    {
//        qDebug() << "Server response error" << "";
//        //emit ServerResponseError();
//        cur_args.clear();
//        return;
//    }
//    if(recieved_args[2] == "1")
//    {
//      //  qDebug() << "LOGIN SUCCESS: " << recieved_args[0] << "";
//        //emit LoginSuccess(recieved_args[0]);
//    }
//    else
//        qDebug() << "Login Failure" << "";
//        //emit LoginFailure();

//    cur_args.clear();

//    qDebug() << "Exit Login";

}


void Client::slogout()
{
    QString serverMessage("slogout|%1|slogout\n");
    tcpSocket->write(serverMessage.arg(uuid).toStdString().c_str());
    tcpSocket->waitForReadyRead();
    tcpSocket -> disconnectFromHost();
    if(!tcpSocket -> waitForDisconnected())
    {
        //Force Connection Close
        tcpSocket -> close();
        qDebug() << "Forced Disconnect" << "";
        //emit ForcedDisconnect();
    }
    qDebug() << "Disconnected" << endl;
    //emit disconnected();

}

void Client::sexit()
{
    QString serverMessage("sexit|%1|sjoin");
    tcpSocket->write(serverMessage.arg(uuid).toStdString().c_str());
    //Force Connection Close
    exitFlag = true;
    tcpSocket -> close();
    qDebug() << "Disconnected" << "";
    //emit disconnected();
}

void Client::ReadSocket()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    qDebug() << QString("ReadSocket %1");
    /*if (tcpSocket->bytesAvailable() < blockSize)
    {
        qDebug() << tcpSocket->bytesAvailable();
        return;
    }*/

    while(tcpSocket->bytesAvailable())
    {
        int bytesRead;

        char nextRead[82];
        qDebug() << "BEFORE";
        bytesRead = in.readRawData(nextRead, 80);
        if(bytesRead == -1)
            nextRead[0] = 0;
        else
            nextRead[bytesRead] = 0;
        qDebug() << QString("ReadSocket %1").arg(bytesRead);
        qDebug() << "AFTER";
        QString temp(nextRead);
        qDebug() << "Temp: " << temp;
        messageSoFar += temp;
        qDebug() << "messageSoFar: " << messageSoFar;
        QStringList commandList = messageSoFar.split("\n",QString::SkipEmptyParts);

        QString tmp;
        foreach(QString command, commandList)
        {
            qDebug() << "Command " << command;
            ServerCommand comm = getMsgStatus(command);
            if(comm != INCOMPLETE && NUM_ARGS[comm] != cur_args.size())
            {
                invalidMessage();
                continue;
            }
            switch(comm)
            {
            case CLOGIN:
                clogin();
                break;
            case CLOGOUT:
                clogout();
                break;
            case CJOIN:
                cjoin();
                break;
            case CLEAVE:
                cleave();
                break;
            case CEXIT:
                cexit();
                break;
            case CSMROOM:
                csmroom();
                break;
            case CRECVMSG:
                crecvmsg();
                break;
            case CULROOM:
                culroom();
                break;
            case CUUID:
                cuuid();
                break;
            case INVALID:
                invalidMessage();
                break;
            case INCOMPLETE: //do nothing, wait for the rest of the message
                break;
            default: break;
                unknownMessage();
            }

            if(comm != INCOMPLETE && comm != INVALID)
            {
                command = "";
            }
            tmp += command;

        }
        messageSoFar = tmp;

    }
    qDebug() << "Done ReadSocket";
}

void Client::handleError(QAbstractSocket::SocketError e)
{
    QString errorMsg("Network Error: %1");
    qDebug() << errorMsg.arg((int)e);
    switch(e)
    {
    case QAbstractSocket::ConnectionRefusedError: break;
    case QAbstractSocket::RemoteHostClosedError: break;
    case QAbstractSocket::HostNotFoundError: break;
    case QAbstractSocket::SocketAccessError: break;
    case QAbstractSocket::SocketResourceError: break;
    case QAbstractSocket::SocketTimeoutError: break;
    case QAbstractSocket::DatagramTooLargeError: break;
    case QAbstractSocket::NetworkError: break;
    case QAbstractSocket::AddressInUseError: break;
    case QAbstractSocket::SocketAddressNotAvailableError: break;
    case QAbstractSocket::UnsupportedSocketOperationError: break;
    case QAbstractSocket::ProxyAuthenticationRequiredError: break;
    case QAbstractSocket::SslHandshakeFailedError: break;
    case QAbstractSocket::UnfinishedSocketOperationError: break;
    case QAbstractSocket::ProxyConnectionRefusedError: break;
    case QAbstractSocket::ProxyConnectionClosedError: break;
    case QAbstractSocket::ProxyConnectionTimeoutError: break;
    case QAbstractSocket::ProxyNotFoundError: break;
    case QAbstractSocket::ProxyProtocolError: break;
    case QAbstractSocket::OperationError: break;
    case QAbstractSocket::SslInternalError: break;
    case QAbstractSocket::SslInvalidUserDataError: break;
    case QAbstractSocket::TemporaryError: break;
    case QAbstractSocket::UnknownSocketError: break;
    default: break;
    }
}

void Client::sjoin(QString channel, QString type)
{
    QString serverMessage("sjoin|%1|%2|%3|sjoin\n");
    tcpSocket->write(serverMessage.arg(QString::number(uuid),channel,type).toStdString().c_str());
}

void Client::sleave(QString channel, QString type)
{
    QString serverMessage("sleave|%1|%2|%3|sleave\n");
    tcpSocket->write(serverMessage.arg(QString::number(uuid),channel,type).toStdString().c_str());
}

void Client::ssmroom(QString room, QString type, QString message)
{
    QString serverMessage("ssmroom|%1|%2|%3|%4\|ssmroom\n");
    tcpSocket->write(serverMessage.arg(QString::number(uuid),room,type,message).toStdString().c_str());
    messageBuffer.enqueue(message);
}

void Client::sulroom(QString room)
{
    QString serverMessage("sulroom|%1|sulroom\n");
    tcpSocket->write(serverMessage.arg(room).toStdString().c_str());
}

void Client::suuid(int uuid)
{
    QString serverMessage("suuid|%1|suuid\n");
    tcpSocket->write(serverMessage.arg(uuid).toStdString().c_str());
}

void Client::loadServerConfig()
{

}

ServerCommand Client::getMsgStatus(QString message)
{
    cur_args = message.split("|");

    if(cur_args.size() >= 2 && (cur_args.first() == cur_args.last()))
    {
        return getCommand(cur_args.first());
    }
    return INCOMPLETE;
}

ServerCommand Client::getCommand(QString command)
{
    if(command == "clogin")
        return CLOGIN;
    if(command == "cjoin")
        return CJOIN;
    if(command == "cleave")
        return CLEAVE;
    if(command == "clogout")
        return CLOGOUT;
    if(command == "cexit")
        return CEXIT;
    if(command == "culroom")
        return CULROOM;
    if(command == "csmroom")
        return CSMROOM;
    if(command == "crecvmsg")
        return CRECVMSG;
    if(command == "cuuid")
        return CUUID;
    return INVALID;
}

void Client::clogin()
{
    qDebug() << "Revieved clogin!";
    bool ok = false;
    QString room = cur_args[1];
    QString uuid = cur_args[2];
    this->uuid = uuid.toInt(&ok);
    if(!ok)
    {
        uuid = -1;
        return;
    }
    QString status = cur_args[3];
    switch(StatusType::getStatus(status))
    {
    case StatusType::STATUS_SUCCESS:
        this->roomName = room;
        emit loginSuccess(room);
        suuid(this->uuid);
        break;
    case StatusType::STATUS_FAILURE:
    case StatusType::STATUS_UST:
    case StatusType::PASSWORD_REQ:
    case StatusType::BAD_UUID:
    case StatusType::UNKNOWN:
    default:
        break;
    }
}

void Client::clogout()
{
    qDebug() << "RUNNING CLOGOUT";
    emit logout();
    userName = "";
    roomName = "";
    messageBuffer.clear();
    uuid = -1;

}

void Client::cjoin()
{
    qDebug() << "RUNNING CJOIN";

    QString status = cur_args[2];
    QString room = cur_args[1];
    switch(StatusType::getStatus(status))
    {
    case StatusType::STATUS_SUCCESS:
        this->roomName = room;
        emit join(room);
        sulroom(room);
        break;
    case StatusType::STATUS_FAILURE:
    case StatusType::STATUS_UST:
        emit userNameTaken(QString("%1 @ %2").arg(this->userName,this->roomName));
    case StatusType::PASSWORD_REQ:
    case StatusType::BAD_UUID:
    case StatusType::UNKNOWN:
    default:
        break;
    }

}

void Client::cexit()
{
    qDebug() << "RUNNING CEXIT";
    emit die();

}

void Client::cleave()
{
    qDebug() << "RUNNING CLEAVE";
    QString status = cur_args[2];
    switch(StatusType::getStatus(status))
    {
    case StatusType::STATUS_SUCCESS:
        emit leave(cur_args[1]);
        break;
    case StatusType::STATUS_FAILURE:
    case StatusType::STATUS_UST:
    case StatusType::PASSWORD_REQ:
    case StatusType::BAD_UUID:
    case StatusType::UNKNOWN:
    default:
        break;
    }

}

void Client::csmroom()
{
    qDebug() << "RUNNING CSMROOM";
    QString status = cur_args[2];
    switch(StatusType::getStatus(status))
    {
    case StatusType::STATUS_SUCCESS:
        emit recievedText(cur_args[1],userName,messageBuffer.dequeue());
        break;
    case StatusType::STATUS_FAILURE:
    case StatusType::STATUS_UST:
    case StatusType::PASSWORD_REQ:
    case StatusType::BAD_UUID:
    case StatusType::UNKNOWN:
    default:
        break;
    }
}

void Client::crecvmsg()
{
    qDebug() << "RUNNING CRECVMSG";
    emit recievedText(cur_args[1],cur_args[2],cur_args[3]);

}

void Client::culroom()
{
    qDebug() << "RUNNING CULROOM";
    QString reply = cur_args[1];
    if(reply.toInt() == -1)
        emit userListFailed();
    else
        emit usersListRoom(reply.split(","));

}

void Client::cuuid()
{
    qDebug() << "RUNNING CUUID";
    QString status = cur_args[1];
    switch(StatusType::getStatus(status))
    {
    case StatusType::STATUS_SUCCESS:
        sulroom(this->roomName);
        break;
    case StatusType::STATUS_FAILURE:
    case StatusType::STATUS_UST:
    case StatusType::PASSWORD_REQ:
    case StatusType::BAD_UUID:
    case StatusType::UNKNOWN:
    default:
        break;
    }

}

void Client::invalidMessage()
{
    qDebug() << "INVALID MESSAGE!!!";

}

void Client::unknownMessage()
{
    qDebug() << "UNRECOGNIZED COMMAND!!";

}
