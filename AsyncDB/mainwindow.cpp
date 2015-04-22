#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVariant>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << "UI:" << QThread::currentThreadId();


    m_asyncDB.createDB("Test", "D:\\test.db");

    m_asyncDB.updateMessage(0, QTime::currentTime().msecsSinceStartOfDay(), "Hello world!", [this](void)
    {
        // here can update UI
        qDebug() << "updateMessage Callback executed! mid = 0";
    });
    m_asyncDB.updateMessage(1, QTime::currentTime().msecsSinceStartOfDay(), "I'm a programmer!", [this](void)
    {
        // here can update UI
        qDebug() << "updateMessage Callback executed! mid = 1";
    });

    m_asyncDB.loadMessage(0, [this](const int id, const qint64 stamp, const QString& content)
    {
        qDebug() << "loadMessage Callback executed! "
                 << id
                 << QTime::fromMSecsSinceStartOfDay(stamp).toString()
                 << content;
    });

    m_asyncDB.loadMessage(1, [this](const int id, const qint64 stamp, const QString& content)
    {
        qDebug() << "loadMessage Callback executed! "
                 << id
                 << QTime::fromMSecsSinceStartOfDay(stamp).toString()
                 << content;
    });

    m_asyncDB.deleteMessage(0, [this](void)
    {
        qDebug() << "deleteMessage Callback executed! mid = 0";
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
