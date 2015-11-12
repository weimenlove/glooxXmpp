TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

unix:!macx: LIBS += -L$$PWD/../lib/ -Wl,-rpath=$$PWD/../lib/ -lgloox -lpthread

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include
