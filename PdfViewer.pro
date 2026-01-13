QT += widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    pdfdocument.cpp

HEADERS += \
    mainwindow.h \
    pdfdocument.h

FORMS += \
    mainwindow.ui

# ---- PDFium ----
INCLUDEPATH += $$PWD/pdfium/include
LIBS += $$PWD/pdfium/lib/pdfium.dll.lib

win32 {
    QMAKE_POST_LINK += copy /Y $$shell_path($$PWD/pdfium/bin/pdfium.dll) $$shell_path($$OUT_PWD)
}


win32:LIBS += -luser32
