QT += network

CONFIG += console c++11

LIBS += -lwsock32

SOURCES += \
    base64.cpp \
    handshake.cpp \
    main.cpp \
    sha1.cpp

HEADERS += \
    base64.h \
    handshake.h \
    sha1.h
