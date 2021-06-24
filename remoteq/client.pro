QT += qml quick network widgets
CONFIG +=c++17
TARGET = client

INCLUDEPATH += include
HEADERS = include/udsclient.h \
    include/message.h

RESOURCES += client.qrc

SOURCES = src/client.cpp \
    src/message.cpp \
    src/udsclient.cpp

MOC_DIR = .objs
OBJECTS_DIR = .objs
RCC_DIR = .qrcs
DESTDIR = bin


