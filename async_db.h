#ifndef ASYNC_DB_H
#define ASYNC_DB_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QVariant>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QMutex>
#include <QList>


#include <functional>

class Worker : public QObject
{
    Q_OBJECT

public slots:
    void doWork(const QSqlQuery &parameter)
    {
        qDebug() << "workerThreadID:" << QThread::currentThreadId();

        /* ... here is the expensive or blocking operation ... */
        QSqlQuery result(parameter);
        bool success = result.exec();
        emit resultReady(success, result);
    }

signals:
    void resultReady(bool success, const QSqlQuery &result);
};

class Controller : public QObject
{
    Q_OBJECT

public:
    Controller()
    {
        Worker *worker = new Worker;
        worker->moveToThread(&m_workerThread);
        connect(&m_workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &Controller::operate, worker, &Worker::doWork);
        connect(worker, &Worker::resultReady, this, &Controller::handleResults);
        m_workerThread.start();
    }

    ~Controller()
    {
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void asyncOperate(QSqlQuery &op,
                       std::function<void(const QSqlQuery &)> handler = std::function<void(const QSqlQuery &)>{})
    {
        m_mutex.lock();
        m_handler.push_back(handler);
        m_mutex.unlock();

        emit operate(op);
    }

signals:
    void operate(const QSqlQuery &op);

private slots:
    void handleResults(bool success, const QSqlQuery &result)
    {
        std::function<void(const QSqlQuery &)> handler;
        m_mutex.lock();
        if(m_handler.size() > 0)
        {
            handler = m_handler.front();
            m_handler.pop_front();
        }
        m_mutex.unlock();

        if(handler && success)
        {
            handler(result);
        }
    }

private:
    QThread m_workerThread;
    QMutex  m_mutex;
    QList<std::function<void(const QSqlQuery &)>> m_handler;
};


class AsyncDataBase : public QObject
{
    Q_OBJECT

public:
    AsyncDataBase()
    {
        qRegisterMetaType<QSqlQuery>("QSqlQuery");
    }
    ~AsyncDataBase()
    {
        if(QSqlDatabase::database(m_connectionName, false).open())
        {
            QSqlDatabase::database(m_connectionName, false).close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);

        m_connectionName = QString::null;
    }

    void createDB( const QString &connectionName,
                   const QString &dbFileName )
    {
        m_connectionName = connectionName;
        QSqlDatabase db  = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.setDatabaseName(dbFileName);

        if(!db.open())
        {
            qDebug() << "DB file name:" << dbFileName;
            qDebug() << "Database Error:" << db.lastError().text();
            return;
        }

        db.exec(QString("CREATE TABLE IF NOT EXISTS '%1' ("
                        "mid BIGINT PRIMARY KEY NOT NULL,"
                        "stamp BIGINT NOT NULL,"
                        "content VARCHAR(128) NOT NULL"
                        ")").arg("message"));

        qDebug() << "DataBase_ThreadID:" << QThread::currentThreadId();
    }

    void loadMessage(int id,
        std::function<void(const int, const qint64, const QString&)> handler = std::function<void(const int, const qint64, const QString&)>{})
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(QString("SELECT * FROM %1 WHERE mid = %2")
                      .arg("message")
                      .arg(id));

        m_controller.asyncOperate(query, [handler](const QSqlQuery &res)
        {
            QSqlQuery result(res);
            while (result.next())
            {
                int mid = result.value("mid").toInt();
                qint64 stamp = result.value("stamp").toLongLong();
                QString content = result.value("content").toString();

                if(handler)
                {
                    handler(mid, stamp, content);
                }
            }
        });
    }

    void updateMessage(int id, qint64 stamp, QString content,
        std::function<void(void)> handler = std::function<void(void)>{})
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(QString("REPLACE INTO %1 (mid, stamp, content) VALUES ("
                              ":mid, :stamp, :content)")
                      .arg("message"));

        query.bindValue(":mid", id);
        query.bindValue(":stamp", stamp);
        query.bindValue(":content", content);

        m_controller.asyncOperate(query, [handler](const QSqlQuery &result)
        {
            Q_UNUSED(result)
            if (handler)
            {
                handler();
            }
        });
    }

    void deleteMessage(int id,
        std::function<void(void)> handler = std::function<void(void)>{})
    {
        QSqlQuery query(QSqlDatabase::database(m_connectionName));
        query.prepare(QString("DELETE FROM %1 WHERE mid = %2")
                      .arg("message")
                      .arg(id));

        m_controller.asyncOperate(query, [handler](const QSqlQuery &result)
        {
            Q_UNUSED(result)
            if (handler)
            {
                handler();
            }
        });
    }

private:
    QString      m_connectionName;
    Controller   m_controller;
};
#endif // ASYNC_DB_H

