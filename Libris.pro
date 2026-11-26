APP_NAME = BerryLeaf
TARGET = BerryLeaf

CONFIG += qt warn_on cascades10
QT += declarative xml network sql gui script

SOURCES += src/main.cpp \
           src/ApplicationUI.cpp \
           src/BookManager.cpp \
           src/BookWorkers.cpp \
           src/TtsManager.cpp \
           src/DictManager.cpp \
           src/OpdsManager.cpp \
           src/AppTranslator.cpp \
           src/EpubParser.cpp \
           src/TxtParser.cpp \
           src/Fb2Parser.cpp \
           src/miniz.c \
           src/pugixml.cpp

HEADERS += src/ApplicationUI.hpp \
           src/BookManager.hpp \
           src/BookWorkers.hpp \
           src/BookChapter.hpp \
           src/TtsManager.hpp \
           src/DictManager.hpp \
           src/OpdsManager.hpp \
           src/AppTranslator.hpp \
           src/EpubParser.hpp \
           src/TxtParser.hpp \
           src/Fb2Parser.hpp \
           src/miniz.h \
           src/pugiconfig.hpp \
           src/pugixml.hpp \
           src/QmlTimer.hpp

INCLUDEPATH += src src/nsrreadercore

CONFIG(simulator, device|simulator) {
    # x86 Simulator Target: Compile stub implementation directly
    DEFINES += NSRREADERCORE_LIBRARY
    SOURCES += src/nsrreadercore_stub.cpp
    HEADERS += src/nsrreadercore/nsrreadercore.h
} else {
    # ARMv7 Device Target: Link native precompiled shared library
    LIBS += -L$$quote($$_PRO_FILE_PWD_)/assets/lib -lnsrreadercore
}

LIBS += -lbbcascadespickers -lbb -lbbsystem -lbbmultimedia -lbps -lbbdevice -lbbplatform

QMAKE_CXXFLAGS += -Wno-psabi
