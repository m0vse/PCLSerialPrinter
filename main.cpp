#include "pclserialprinter.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PCLSerialPrinter w;
    w.show();
    return a.exec();
}
