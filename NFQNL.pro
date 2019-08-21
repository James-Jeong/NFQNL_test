TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

HEADERS += \
    stdafx.h \
    libnet-asn1.h \
    libnet-functions.h \
    libnet-headers.h \
    libnet-macros.h \
    libnet-structures.h \
    libnet-types.h

LIBS += -lnetfilter_queue
