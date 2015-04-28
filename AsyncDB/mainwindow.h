#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "async_db.h"
#include "async_db2.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    AsyncDataBase m_asyncDB;
    AsyncDataBase2 *m_asyncDB2;
};

#endif // MAINWINDOW_H
