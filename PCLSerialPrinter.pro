# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

QT       = core gui serialport widgets
TEMPLATE = app
TARGET = PCLSerialPrinter
DESTDIR = ./Debug
CONFIG += debug
win32:LIBS += -L"." -lshell32
DEPENDPATH += .
MOC_DIR += .
OBJECTS_DIR += debug
UI_DIR += .
RCC_DIR += .
include(PCLSerialPrinter.pri)
