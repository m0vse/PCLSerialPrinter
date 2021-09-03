#include "pclserialprinter.h"

PCLSerialPrinter::PCLSerialPrinter(QWidget* parent)
    : QMainWindow(parent)
{
    QGuiApplication::setApplicationDisplayName("PCLSerialPrinter");
    QGuiApplication::setApplicationName(QString("PCLSerialPrinter"));

    ui.setupUi(this);

    setWindowTitle(QString("PCLSerialPrinter"));

    qRegisterMetaType<QSerialPortInfo>();

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : infos) {
        ui.portCombo->addItem(info.portName(), QVariant::fromValue(info));
    }
    ui.portCombo->setCurrentText("COM1");

    ui.baudCombo->addItem(QString("115200"), QSerialPort::Baud115200);
    ui.baudCombo->addItem(QString("57600"), QSerialPort::Baud57600);
    ui.baudCombo->addItem(QString("38400"), QSerialPort::Baud38400);
    ui.baudCombo->addItem(QString("19200"), QSerialPort::Baud19200);
    ui.baudCombo->addItem(QString("9600"), QSerialPort::Baud9600);
    ui.baudCombo->addItem(QString("4800"), QSerialPort::Baud4800);
    ui.baudCombo->addItem(QString("2400"), QSerialPort::Baud2400);
    ui.baudCombo->addItem(QString("1200"), QSerialPort::Baud1200);
    ui.baudCombo->setCurrentText("1200");


    ui.bitsCombo->addItem(QString("8"), QSerialPort::Data8);
    ui.bitsCombo->addItem(QString("7"), QSerialPort::Data7);

    ui.stopCombo->addItem(QString("1"), QSerialPort::OneStop);
    ui.stopCombo->addItem(QString("2"), QSerialPort::TwoStop);

    ui.parityCombo->addItem(QString("None"), QSerialPort::NoParity);
    ui.parityCombo->addItem(QString("Odd"), QSerialPort::OddParity);
    ui.parityCombo->addItem(QString("Even"), QSerialPort::EvenParity);

    ui.handshakeCombo->addItem(QString("None"), QSerialPort::NoFlowControl);
    ui.handshakeCombo->addItem(QString("Software"), QSerialPort::SoftwareControl);
    ui.handshakeCombo->addItem(QString("Hardware"), QSerialPort::HardwareControl);
    ui.handshakeCombo->setCurrentText("Hardware");

    ui.scaleCombo->addItem(QString("1:1"), 1);
    ui.scaleCombo->addItem(QString("2:1"), 2);
    ui.scaleCombo->addItem(QString("3:1"), 3);
    ui.scaleCombo->setCurrentText("1:1");

    color.setCurrentColor(Qt::black);
    connect(&port, &QSerialPort::readyRead, this, &PCLSerialPrinter::readData);

    ui.graphicsView->setScene(&scene);
    scene.addItem(&item);
}

PCLSerialPrinter::~PCLSerialPrinter()
{
    delete image;
    delete timer;
    if (port.isOpen()) {
        port.close();
    }
}
void PCLSerialPrinter::on_connectBtn_clicked()
{
    if (ui.connectBtn->text() == "Connect")
    {
        port.setPort(ui.portCombo->currentData().value<QSerialPortInfo>());

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&PCLSerialPrinter::update));

        if (port.open(QIODevice::ReadWrite)) {
            port.setBaudRate(ui.baudCombo->currentData().value<QSerialPort::BaudRate>());
            port.setDataBits(ui.bitsCombo->currentData().value<QSerialPort::DataBits>());
            port.setStopBits(ui.stopCombo->currentData().value<QSerialPort::StopBits>());
            port.setParity(ui.parityCombo->currentData().value<QSerialPort::Parity>());
            port.setFlowControl(ui.handshakeCombo->currentData().value<QSerialPort::FlowControl>());
            ui.connectBtn->setText("Disconnect");
        }
        else {
            ui.statusBar->showMessage("Failed to open " + port.portName() + ": " + QString(port.errorString()), 5000);
        }
    }
    else
    {
        if (port.isOpen()) {
            port.close();
            ui.connectBtn->setText("Connect");
        }
    }

}

void PCLSerialPrinter::on_colourBtn_clicked()
{
    color.show();
}

void PCLSerialPrinter::on_scaleCombo_currentIndexChanged(int index)
{
    scale = ui.scaleCombo->currentData().value<int>();
    if (image != Q_NULLPTR) {
        item.setPixmap(QPixmap::fromImage(*image).scaled(image->width()*scale,image->height()*scale,Qt::KeepAspectRatio));
        scene.setSceneRect(0,0,image->width()*scale,image->height()*scale);
        //ui.graphicsView->fitInView(scene.sceneRect(),Qt::KeepAspectRatio);
    }
    else {
        ui.statusBar->showMessage("Image has been deleted!",5000);
    }
}



void PCLSerialPrinter::readData()
{
    lastData=QDateTime::currentDateTime();
    if (port.bytesAvailable() && !timer->isActive())
    {
        timer->start(150);
    }

    while (port.bytesAvailable()){
        //const QByteArray in = port.readAll();
        mutex.lock();
        imageData.append(port.read(1));
        mutex.unlock();
    }
}

void PCLSerialPrinter::update()
{

    if (lastData.msecsTo(QDateTime::currentDateTime())>500) {
        // Something bad has happened as we are still running but haven't received any data for over 500ms
        // The most likely cause is that the transfer got interrupted.
        timer->stop();
        imageData.clear();
        linenr=0;
        currentPosition=0;
        active=false;
        return;
    }

    mutex.lock();
    QString data = QString::fromStdString(imageData.mid(currentPosition).toStdString());
    mutex.unlock();
    QRegularExpression re("\x1b(?<prefix>.)(?<cmd>[a-z])(?<num>[0-9]{0,4}?)(?<suffix>[A-Z])");
    QRegularExpressionMatchIterator it = re.globalMatch(data);

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString prefix=match.captured("prefix");
        QString cmd=match.captured("cmd");
        int num=match.captured("num").toInt();
        QString suffix=match.captured("suffix");

        ui.debugText->appendPlainText(prefix+cmd+QString::number(num)+suffix);

        if (prefix=="*" && cmd=="r" && suffix=="B")
        {
            finished = true;
        }
        else if (prefix=="*" && cmd=="r" && suffix =="A")
        {
            active=true;
        }
        else if (prefix=="*" && cmd=="b" && suffix =="W")
        {

            if (num>imageWidth) {
                imageWidth=num;
            }

            if (linenr==0) {
                if (image == Q_NULLPTR) {
                    delete image;
                }
                image = new QImage((imageWidth*8),(imageWidth*8),QImage::Format_RGB888);
                image->fill(Qt::white);
                imageHeight=0;
            }

            startImageData=currentPosition+match.capturedEnd("suffix");
            if (imageData.length()>startImageData+num)
            {
                /*if (it.hasNext()) {
                    QRegularExpressionMatch test = it.peekNext();
                    if (test.capturedStart("prefix")-match.capturedEnd("suffix")<num)
                    {
                        num=test.capturedStart("prefix")-match.capturedEnd("suffix");
                    }
                }*/
                imageLine = imageData.mid(startImageData,num);
                //QPainter painter(image);
                //painter.setBrush(Qt::black);
                //painter.setPen(Qt::black);
                ui.debugText->appendPlainText("I: "+QString::number(imageLine.length()));
                for (int chars=0;chars<imageLine.length();chars++)
                {
                    unsigned char count=7;
                    for (unsigned char bits=0;bits<8;bits++)
                    {
                        unsigned char tempnum = (unsigned char)imageLine[chars] >> bits & 0x01;
                        if (tempnum==0x01) {
                            //painter.drawRect(((chars*8)+count)*2,linenr*2,2,2);
                            image->setPixelColor((chars*8)+count,linenr,color.currentColor());
                        }
                        count--;
                    }
                }
                //painter.end();
                linenr++;
                //item.setPixmap(QPixmap::fromImage(*image));
                item.setPixmap(QPixmap::fromImage(*image).scaled(image->width()*scale,image->height()*scale,Qt::KeepAspectRatio));
                imageHeight++;
                currentPosition=startImageData+num;
            } else {
                break;
            }

        }
    }

    if (active && !finished) {
        if (ui.progressBar->value() >= ui.progressBar->maximum()) {
            ui.progressBar->setValue(0);
        }
        else {
            ui.progressBar->setValue(ui.progressBar->value() + 1);
        }
    }
    else if (finished) {
        active = false;
        finished=false;

        ui.progressBar->setValue(100);

        ui.statusBar->showMessage("Width: "+QString::number(imageWidth*8)+", Height: "+QString::number(imageHeight), 5000);

        timer->stop();

        QImage copy = image->copy(0,0,imageWidth*8,imageHeight);
        delete image;
        image = new QImage(copy.copy());
        item.setPixmap(QPixmap::fromImage(*image).scaled(image->width()*scale,image->height()*scale,Qt::KeepAspectRatio));

        imageData.clear();
        linenr=0;
        currentPosition=0;

        //QSaveFile file("imagedata.bin");
        //file.open(QIODevice::WriteOnly);
        //file.write(imageData);
        //file.commit();
        //imageData.clear();
    }

}
