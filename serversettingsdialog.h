#ifndef SERVERSETTINGSDIALOG_H
#define SERVERSETTINGSDIALOG_H

#include <QDialog>

class QRegExpValidator;

namespace Ui {
class ServerSettingsDialog;
}

class ServerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerSettingsDialog(QWidget *parent = 0, QString address = "", QString port = "");
    ~ServerSettingsDialog();

    QString hostName, port;

public slots:
    void updateMembers();
private:
    Ui::ServerSettingsDialog *ui;
    QRegExpValidator *hostNameValidator, *portValidator;
};

#endif // SERVERSETTINGSDIALOG_H
