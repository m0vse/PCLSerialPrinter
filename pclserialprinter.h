#pragma once

#include <QtWidgets/QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QVariant>
#include <QByteArray>
//#include <QTextCodec>
#include <QSaveFile>
#include <QTimer>
#include <QMutex>
#include <QColorDialog>
#include <QDateTime>
#include "ui_pclserialprinter.h"

class PCLSerialPrinter : public QMainWindow
{
    Q_OBJECT

public:
    PCLSerialPrinter(QWidget *parent = Q_NULLPTR);
    ~PCLSerialPrinter();
public slots:
    void on_connectBtn_clicked();
    void on_colourBtn_clicked();
    void on_scaleCombo_currentIndexChanged(int index);
    void readData();
    void update();

private:
    Ui::PCLSerialPrinter ui;
    QSerialPort port;
    bool active = false;
    bool finished = false;
    int imageWidth = 0;
    int imageHeight = 0;
    QByteArray imageData;
    int currentPosition=0;
    int startImageData=0;
    int endImageData=0;
    int linenr=0;
    QByteArray imageLine;
    QGraphicsScene scene;
    QPixmap pixmap;
    QImage *image = Q_NULLPTR;
    QGraphicsPixmapItem item;
    QMutex mutex;
    QTimer *timer;
    QColorDialog color;
    int scale=1;
    QDateTime lastData;
};


Q_DECLARE_METATYPE(QSerialPortInfo);
