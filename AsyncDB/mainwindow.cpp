#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVariant>
#include <QTime>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList()
                                               <<QString("ID")
                                               <<QString("TODO"));
    qDebug() << "UI:" << QThread::currentThreadId();

    ui->label_2->hide();
    ui->label_3->hide();
    ui->lineEditFindID->hide();
    ui->lineEditDelID->hide();
    ui->pushButtonFind->hide();
    ui->pushButtonDelete->hide();

    // 第一种异步数据库
    m_asyncDB.createDB("Test", "D:\\test.db");

    connect(ui->pushButtonAdd, &QPushButton::clicked, [this]()
    {
        static int id = 0;
        m_asyncDB.updateMessage(id, QTime::currentTime().msecsSinceStartOfDay(), "Hello world!", [this, &id](void)
        {
            // here can update UI
            qDebug() << "updateMessage Callback executed! mid = " << id;
            int rows = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(rows+1);
            ui->tableWidget->setItem(rows, 0, new QTableWidgetItem(QString("%1").arg(id)));
            ui->tableWidget->setItem(rows, 1, new QTableWidgetItem(ui->lineEditMessage->text()));
            id ++;
        });
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


    // 第二种异步数据库
    m_asyncDB2 = new AsyncDataBase2();
    if(m_asyncDB2)
    {
        m_asyncDB2->start();
        m_asyncDB2->createDB("Test2", "D:\\test2.db");

        m_asyncDB2->appendData(Data(0, "Hello"));
        m_asyncDB2->appendData(Data(1, "World"));
        m_asyncDB2->appendData(Data(2, "!"));
        m_asyncDB2->appendData(Data(3, "Haha"));

        m_asyncDB2->appendRemoveData(Data(1, "World"));
        m_asyncDB2->appendRemoveData(Data(3, "Haha"));
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}
