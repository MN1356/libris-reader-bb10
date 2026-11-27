APP_NAME = Libris
TARGET = Libris

CONFIG += qt warn_on cascades10
QT += declarative xml network sql gui script

SOURCES += src/main.cpp \
           src/ApplicationUI.cpp \
           src/BookManager.cpp \
           src/BookWorkers.cpp \
           src/TlsHttpClient.cpp \
           src/TtsManager.cpp \
           src/DictManager.cpp \
           src/OpdsManager.cpp \
           src/AppTranslator.cpp \
           src/EpubParser.cpp \
           src/TxtParser.cpp \
           src/Fb2Parser.cpp \
           src/CbzParser.cpp \
           src/MobiParser.cpp \
           src/PdfParser.cpp \
           src/WiFiDropServer.cpp \
           src/miniz.c \
           src/pugixml.cpp

HEADERS += src/ApplicationUI.hpp \
           src/BookManager.hpp \
           src/BookWorkers.hpp \
           src/TlsHttpClient.hpp \
           src/BookChapter.hpp \
           src/TtsManager.hpp \
           src/DictManager.hpp \
           src/OpdsManager.hpp \
           src/AppTranslator.hpp \
           src/EpubParser.hpp \
           src/TxtParser.hpp \
           src/Fb2Parser.hpp \
           src/CbzParser.hpp \
           src/MobiParser.hpp \
           src/PdfParser.hpp \
           src/WiFiDropServer.hpp \
           src/miniz.h \
           src/pugiconfig.hpp \
           src/pugixml.hpp \
           src/ImageUriUtil.hpp \
           src/BuildInfo.hpp \
           src/QmlTimer.hpp

INCLUDEPATH += src src/pdfcore

CONFIG(simulator, device|simulator) {
    # x86 Simulator Target: Compile stub implementation directly
    DEFINES += NSRREADERCORE_LIBRARY
    SOURCES += src/pdfcore/pdfcore_stub.cpp
    HEADERS += src/pdfcore/pdfcore.h
} else {
    # ARMv7 Device Target: Link native precompiled shared library
    LIBS += -L$$quote($$_PRO_FILE_PWD_)/assets/lib -lnsrreadercore
}

LIBS += -lbbcascadespickers -lbb -lbbsystem -lbbmultimedia -lbps -lbbdevice -lbbplatform -lsocket -lssl -lcrypto

QMAKE_CXXFLAGS += -Wno-psabi

# --- eSpeak-NG offline TTS (vendored C) ---
SOURCES += src/EspeakEngine.cpp \
           src/espeak-ng/lib/common.c \
           src/espeak-ng/lib/compiledata.c \
           src/espeak-ng/lib/compiledict.c \
           src/espeak-ng/lib/dictionary.c \
           src/espeak-ng/lib/encoding.c \
           src/espeak-ng/lib/error.c \
           src/espeak-ng/lib/espeak_api.c \
           src/espeak-ng/lib/ieee80.c \
           src/espeak-ng/lib/intonation.c \
           src/espeak-ng/lib/klatt.c \
           src/espeak-ng/lib/langopts.c \
           src/espeak-ng/lib/mnemonics.c \
           src/espeak-ng/lib/numbers.c \
           src/espeak-ng/lib/phoneme.c \
           src/espeak-ng/lib/phonemelist.c \
           src/espeak-ng/lib/readclause.c \
           src/espeak-ng/lib/setlengths.c \
           src/espeak-ng/lib/soundicon.c \
           src/espeak-ng/lib/spect.c \
           src/espeak-ng/lib/speech.c \
           src/espeak-ng/lib/ssml.c \
           src/espeak-ng/lib/synthdata.c \
           src/espeak-ng/lib/synthesize.c \
           src/espeak-ng/lib/tr_languages.c \
           src/espeak-ng/lib/translate.c \
           src/espeak-ng/lib/translateword.c \
           src/espeak-ng/lib/voices.c \
           src/espeak-ng/lib/wavegen.c \
           src/espeak-ng/ucd/case.c \
           src/espeak-ng/ucd/categories.c \
           src/espeak-ng/ucd/ctype.c \
           src/espeak-ng/ucd/proplist.c \
           src/espeak-ng/ucd/scripts.c \
           src/espeak-ng/ucd/tostring.c
HEADERS += src/EspeakEngine.hpp
INCLUDEPATH += src/espeak-ng/qnx_inc src/espeak-ng/include src/espeak-ng/ucd/include src/espeak-ng/lib
QMAKE_CFLAGS += -O2 -std=gnu99 -fwrapv -fvisibility=hidden -include src/espeak-ng/qnx_inc/wctype_shim.h -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-missing-field-initializers
DEFINES += LIBESPEAK_NG_EXPORT=1
LIBS += -lm
