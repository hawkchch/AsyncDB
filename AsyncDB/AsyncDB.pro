#-------------------------------------------------
#
# Project created by QtCreator 2015-03-20T16:10:45
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ThreadTest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    async_db2.cpp

HEADERS  += mainwindow.h \
    async_db.h \
    async_db2.h

FORMS    += mainwindow.ui


CONFIG += c++11
