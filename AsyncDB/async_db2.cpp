#include "async_db2.h"
#include <QSqlError>



AsyncDataBase2::AsyncDataBase2()
{

}

AsyncDataBase2::~AsyncDataBase2()
{
    m_bStop = true;
    QSqlDatabase::database(m_connectionName, false).close();
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName = QString::null;
    wait();
}

bool AsyncDataBase2::createDB(const QString &connectionName, const QString &dbFileName)
{
    m_connectionName = connectionName;
    QSqlDatabase db  = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbFileName);

    if(!db.open())
    {
        qDebug() << "DB file name:" << dbFileName;
        qDebug() << "Database Error:" << db.lastError().text();
        return false;
    }

    db.exec(QString("CREATE TABLE IF NOT EXISTS '%1' ("
                    "id BIGINT PRIMARY KEY NOT NULL,"
                    "info VARCHAR(128) NOT NULL"
                    ")").arg("Data"));

    qDebug() << "DataBase_ThreadID:" << QThread::currentThreadId();

    m_dbReady = true;

    return true;
}

bool AsyncDataBase2::dbIsOpen()
{
    if(m_connectionName.isEmpty())
    {
        return false;
    }

    return QSqlDatabase::database(m_connectionName, false).isOpen();
}

bool AsyncDataBase2::insertData(const Data &data)
{
    if(!dbIsOpen())
    {
        qDebug() << "Insert dept to DB error: DB is not opened";
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QString("REPLACE INTO %1 (id, info) VALUES ("
                          ":id, :info)").arg("Data"));
    query.bindValue(":id", data.id);
    query.bindValue(":info", data.info);
    if(!query.exec())
    {
        qDebug() << "exec query error: " << query.lastError().text() << "executedQuery: " << query.lastQuery();
        return false;
    }

    return true;
}

bool AsyncDataBase2::removeData(const Data &data)
{
    QSqlQuery query(QString("DELETE FROM %1 WHERE id = %2").arg("Data").arg(data.id),
                    QSqlDatabase::database(m_connectionName));
    if(!query.exec())
    {
        qDebug() << "exec query error: " << query.lastError().text() << "executedQuery: " << query.lastQuery();
        return false;
    }

    return true;
}

void AsyncDataBase2::appendInsertData(const Data &data)
{
    m_mutexDatas.lock();
    m_waitingForInsertDatas.push_back(data);
    m_mutexDatas.unlock();
}

void AsyncDataBase2::appendRemoveData(const Data &data)
{
    m_mutexRemoveDatas.lock();
    m_waitingForRemoveDatas.push_back(data);
    m_mutexRemoveDatas.unlock();
}

bool AsyncDataBase2::isIdle()
{
    if(!m_waitingForInsertDatas.isEmpty())
    {
        return false;
    }

    if(!m_waitingForRemoveDatas.isEmpty())
    {
        return false;
    }

    return true;
}

void AsyncDataBase2::stop()
{
    m_bStop = true;

    m_mutexDatas.lock();
    m_waitingForInsertDatas.clear();
    m_mutexDatas.unlock();

    m_mutexRemoveDatas.lock();
    m_waitingForRemoveDatas.clear();
    m_mutexRemoveDatas.unlock();
}

void AsyncDataBase2::run()
{
    qDebug() << "Run_ThreadID:" << QThread::currentThreadId();
    m_bStop = false;
    while(m_dbReady && !m_bStop)
    {
        TransactionCall<Data>(m_waitingForInsertDatas,
                              m_mutexDatas,
                              std::bind(&AsyncDataBase2::insertData, this, std::placeholders::_1),
                              std::function<void(const Data&)>{},
                              QString("insert data"));

        TransactionCall<Data>(m_waitingForRemoveDatas,
                              m_mutexRemoveDatas,
                              std::bind(&AsyncDataBase2::removeData, this, std::placeholders::_1),
                              std::function<void(const Data&)>{},
                              QString("remove data"));

        if(isIdle())
        {
            msleep(5);
        }
    }
}
