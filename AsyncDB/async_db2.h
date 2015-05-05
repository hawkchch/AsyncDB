#ifndef ASYNC_DB2_H
#define ASYNC_DB2_H

#include <QMutex>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QString>

#include <binders.h>
#include <functional>

struct Data
{
    Data(int i, QString infos)
    {
        id = i;
        info = infos;
    }
    int     id;
    QString info;
};

class AsyncDataBase2 : public QThread
{
    Q_OBJECT

public:
    AsyncDataBase2();
    ~AsyncDataBase2();

    bool createDB( const QString &connectionName,
                   const QString &dbFileName );

    // 把容器内所有元素取出组成一个事务，进行数据库操作
    template<typename T>
    bool TransactionCall(QList<T> &container,
                QMutex &mutex,
                std::function<bool(const T&)> caller,
                std::function<void(const T&)> completor = std::function<void(const T&)>{},
                QString info = QString::null)
    {
        bool ret = false;
        if(container.isEmpty())
        {
            return ret;
        }

        mutex.lock();
        auto container_backup = container;
        mutex.unlock();

        qDebug() << QString("%1 Transaction Begin.").arg(info);

        QSqlQuery query(QSqlDatabase::database(m_connectionName));

        // 开启事务
        if (!query.exec(QString("begin transaction")))
        {
            return ret;
        }

        // 构造事务
        foreach(auto elem, container_backup)
        {
            caller(elem);
        }

        // 提交事务
        if ( query.exec(QString("commit transaction")) )
        {
            mutex.lock();
            for(int i=0; i<container_backup.size(); i++)
            {
                container.removeFirst();
            }
            mutex.unlock();

            // 事务完成后的结束函数
            if (completor)
            {
                foreach(auto elem, container_backup)
                {
                    completor(elem);
                }
            }

            ret = true;
            qDebug() << QString("%1 Transaction End. Size = %2.").arg(info).arg(container_backup.size());
        }
        else
        {
            // 回滚事务
            query.exec(QString("rollback transaction"));
            ret = false;
        }

        return ret;
    }

    bool dbIsOpen();

    void appendInsertData(const Data& data);

    void appendRemoveData(const Data& data);

    bool isIdle();

    void stop();


protected:
    void run();

    bool insertData(const Data& data);

    bool removeData(const Data& data);
private:

    bool            m_bStop;
    bool            m_dbReady;
    QString         m_connectionName;
    mutable QMutex  m_mutexDatas;
    mutable QMutex  m_mutexRemoveDatas;
    QList<Data>     m_waitingForInsertDatas;
    QList<Data>     m_waitingForRemoveDatas;

};
#endif // ASYNC_DB2_H
