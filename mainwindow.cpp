#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    server = new QTcpServer(this);
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::on_disconnectFromServer_clicked);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::serverGotNewConnection);
}

MainWindow::~MainWindow()
{
    delete server;
    delete socket;

    delete ui;
}

/*  Make current instance work as a server */
void MainWindow::on_WorkAsServerRadioButton_clicked()
{
    if (serverModeActive == true)
        return;
    serverModeActive = true;

    ui->statusbar->showMessage("Application will now be working in server mode");
    if (ui->disconnectFromServer->isEnabled()) // if it is not enabled - that means we have disconnected already or in process
        ui->disconnectFromServer->click();
    ui->choose_picture_panel->setDisabled(true);

    ui->DesiredServerPort->setDisabled(true);
    ui->DesiredServerNameOrIp->setDisabled(true);
    ui->connectToServer->setDisabled(true);

    ui->ServerPort->setEnabled(true);
    ui->StartStopPortListeningButton->setEnabled(true);
}

/*  Make current instance work as a client */
void MainWindow::on_WorkAsClientRadioButton_clicked()
{
    if (serverModeActive == false)
        return;
    serverModeActive = false;

    ui->statusbar->showMessage("Application will now be working in client mode");
    if (server->isListening())
        server->close();
    ui->choose_picture_panel->setEnabled(true);

    ui->ServerPort->setDisabled(true);
    ui->StartStopPortListeningButton->setDisabled(true);
    ui->StartStopPortListeningButton->setChecked(false);
    ui->StartStopPortListeningButton->setText("Listening is off");

    ui->DesiredServerPort->setEnabled(true);
    ui->DesiredServerNameOrIp->setEnabled(true);
    ui->connectToServer->setEnabled(true);
    ui->disconnectFromServer->setDisabled(true);
}

/*  Disconnect from server */
void MainWindow::on_disconnectFromServer_clicked()
{
    ui->disconnectFromServer->setDisabled(true);
    socket->disconnectFromHost();
    if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected(5000))
    {
        ui->statusbar->showMessage("Disconnected from server.");
    }
    else
    {
        socket->abort();
        ui->statusbar->showMessage("Connection abort, disconnect timed out.");
    }
    ui->DesiredServerPort->setEnabled(true);
    ui->DesiredServerNameOrIp->setEnabled(true);
    ui->connectToServer->setEnabled(true);
}

/*  Connect to server */
void MainWindow::on_connectToServer_clicked()
{
    ui->connectToServer->setDisabled(true);
    ui->DesiredServerPort->setDisabled(true);
    ui->DesiredServerNameOrIp->setDisabled(true);

    bool convRes = false;
    QRegExp invalidPortNumber("^(\\d*)\\D(.*)");
    quint16 serverPort = ui->DesiredServerPort->text().toUShort(&convRes, 10);
    if (ui->DesiredServerPort->text().contains(invalidPortNumber) == true || convRes == false)
    {
        ui->statusbar->showMessage("Invalid port number (or it is too big). It also should only contain digits.");
        ui->connectToServer->setEnabled(true);
        ui->DesiredServerPort->setEnabled(true);
        ui->DesiredServerNameOrIp->setEnabled(true);
        return;
    }
    socket->connectToHost(ui->DesiredServerNameOrIp->text(), serverPort);
    if (socket->state() == QAbstractSocket::ConnectedState || socket->waitForConnected(5000))
    {
        ui->statusbar->showMessage("Connection established.");
        ui->disconnectFromServer->setEnabled(true);
    }
    else
    {
        ui->statusbar->showMessage("Error occured, while trying to connect to server. Try different settings or try again.");
        socket->abort();
        ui->connectToServer->setEnabled(true);
        ui->DesiredServerPort->setEnabled(true);
        ui->DesiredServerNameOrIp->setEnabled(true);
    }
}

void MainWindow::writeDataToSocket()
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << (qint64)0;
    ds << picture;
    ds.device()->seek(0);
    ds << (qint64)(ba.size() - sizeof(qint64));
    socket->write(ba);
}

void MainWindow::serverGotNewConnection()
{
    if (server_socket != nullptr) // limiting connections to 1, if i am right
        return;
    server_socket = server->nextPendingConnection();
    connect(server_socket, &QTcpSocket::readyRead, this, &MainWindow::readDataFromSocket);
    connect(server_socket, &QTcpSocket::disconnected, [=] () { server_socket = nullptr; });
}

void MainWindow::readDataFromSocket()
{
    static qint64 len = 0;
    QTcpSocket *sc = qobject_cast<QTcpSocket *>(sender());
    QDataStream recieved(sc);
    while(!len)
    {
        if (sc->bytesAvailable() < (qint64)sizeof(qint64))
        {
            return;
        }
        recieved >> len;
    }
    if (sc->bytesAvailable() < len - (qint64)sizeof(qint64))
    {
        return;
    }
    recieved >> picture;
    len = 0;
    sc->deleteLater();
    server_socket = nullptr;
    if (scene != nullptr)
        delete scene;
    scene = new QGraphicsScene(ui->PictureView);
    scene->addPixmap(picture);
    ui->PictureView->setScene(scene);
    QResizeEvent e(MainWindow::size(), MainWindow::size());
    this->resizeEvent(&e);
    ui->PictureView->show();
}

/*  Send picture to server */
void MainWindow::on_SendPictureButton_clicked()
{
    if (scene == nullptr)
        return;
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        writeDataToSocket();
    }
    else
    {
        if (socket->state() == QAbstractSocket::UnconnectedState)
        {
            ui->connectToServer->click();
            if (!socket->waitForConnected(5000))
                ui->statusbar->showMessage("Cannot connect to server to send data. Operation terminated. Try again");
            writeDataToSocket();
        }
        else
        {
            if (!socket->waitForConnected(5000))
                ui->statusbar->showMessage("Cannot connect to server to send data. Socket is currently in undefined state. Try again");
            return;
        }
    }
    on_ClearButton_clicked();
}

/*  Enable or disable port listening */
void MainWindow::on_StartStopPortListeningButton_clicked(bool checked)
{
    QCheckBox *cb = qobject_cast<QCheckBox*>(sender());
    if (checked == true)
    {
        bool convRes = false;
        QRegExp invalidPortNumber("^(\\d*)\\D(.*)");
        quint16 portToListen = ui->ServerPort->text().toUShort(&convRes, 10);
        if (ui->ServerPort->text().contains(invalidPortNumber) == true || convRes == false)
        {
            ui->statusbar->showMessage("Invalid port number (or it is too big). It also should only contain digits.");
            cb->setChecked(false);
            return;
        }
        if (server->listen(QHostAddress::Any, portToListen))
        {
            cb->setText("Listening is on");
            ui->statusbar->showMessage("Server is now listening to port \"" + QString::number(server->serverPort()) + "\".");
        }
        else
        {
            ui->statusbar->showMessage("Error while trying to listen to port \"" + QString::number(portToListen) + "\".");
            cb->setChecked(false);
            return;
        }
    }
    if (checked == false)
    {
        cb->setText("Listening is off");
        if (server->isListening())
            server->close();
        ui->statusbar->showMessage("Server has stopped listening for incomming connections now.");
    }
}

/*  Adjust picture size to qgraphicsview */
void MainWindow::resizeEvent(QResizeEvent *e)
{
    e->accept();
    if (scene != nullptr)
    {
        qreal resultingScale;
        if (scene->width() * ui->PictureView->transform().m11() / (qreal) ui->PictureView->width() >
                scene->height() * ui->PictureView->transform().m22() / (qreal) ui->PictureView->height())
        {
            resultingScale = ((qreal) ui->PictureView->width()) / (scene->width() * ui->PictureView->transform().m11());
            ui->PictureView->scale(resultingScale, resultingScale);
        }
        else
        {
            resultingScale = ((qreal) ui->PictureView->height()) / (scene->height() * ui->PictureView->transform().m22());
            ui->PictureView->scale(resultingScale, resultingScale);
        }
    }
}

/*  Open dialog to choose picture from filesystem */
void MainWindow::on_OpenButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Image", QDir::currentPath(), "Image Files (*.png *.jpg *.bmp)");
    if (fileName == "")
        return;
    if (!picture.load(fileName))
    {
        ui->statusbar->showMessage("Failed to load picture.");
        return;
    }
    if (scene != nullptr)
        this->on_ClearButton_clicked();
    ui->statusbar->showMessage("Picture has been load.");
    ui->PathToPicture->setText(fileName);
    scene = new QGraphicsScene(ui->PictureView);
    scene->addPixmap(picture);
    ui->PictureView->setScene(scene);
    QResizeEvent e(MainWindow::size(), MainWindow::size());
    this->resizeEvent(&e);
    ui->PictureView->show();
}

/*  Clear view and path to picture */
void MainWindow::on_ClearButton_clicked()
{
    ui->PathToPicture->setText("");
    delete scene;
    scene = nullptr;
    ui->statusbar->showMessage("Workspace has been cleared.");
}
