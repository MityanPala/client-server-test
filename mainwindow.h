#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QCheckBox>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QResizeEvent>
#include <QObject>
#include <QDataStream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_WorkAsServerRadioButton_clicked();
    void on_WorkAsClientRadioButton_clicked();
    void on_SendPictureButton_clicked();
    void on_ClearButton_clicked();
    void on_OpenButton_clicked();
    void on_StartStopPortListeningButton_clicked(bool checked);
    void on_disconnectFromServer_clicked();
    void on_connectToServer_clicked();
    void serverGotNewConnection();

private:
    Ui::MainWindow *ui;

    void resizeEvent(QResizeEvent *e);

    bool serverModeActive = false; // flag to block multiple presses of the same radiobutton
    QPixmap picture;
    QGraphicsScene *scene = nullptr;

    QTcpServer *server = nullptr;
    QTcpSocket *server_socket = nullptr;
    QTcpSocket *socket = nullptr;
    void readDataFromSocket();
    void writeDataToSocket();
};
#endif // MAINWINDOW_H
