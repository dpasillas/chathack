#include "serversettingsdialog.h"
#include "ui_serversettingsdialog.h"
#include <QDebug>

ServerSettingsDialog::ServerSettingsDialog(QWidget *parent, QString address, QString port) :
    QDialog(parent),
    ui(new Ui::ServerSettingsDialog),
    hostName(address), port(port)
{
    ui->setupUi(this);
    this->setWindowTitle("Server Configuration");
    hostNameValidator = new QRegExpValidator(QRegExp("[\\da-zA-Z]+(\\.[\\da-zA-Z]+)*"));
    portValidator      = new QRegExpValidator(QRegExp("\\d{1,5}"));

    ui->hostAddressLine->setText(address);
    ui->portLine->setText(port);

    ui->hostAddressLine->setValidator(hostNameValidator);
    ui->portLine->setValidator(portValidator);

    connect(this,SIGNAL(accepted()),this,SLOT(updateMembers()));
}

ServerSettingsDialog::~ServerSettingsDialog()
{
    delete ui;
    delete hostNameValidator;
    delete portValidator;
}

void ServerSettingsDialog::updateMembers()
{
    this->hostName = ui->hostAddressLine->text();
    this->port = ui->portLine->text();
}
