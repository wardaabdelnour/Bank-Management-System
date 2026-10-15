QT       += core gui widgets

CONFIG   += c++17

TARGET    = BankGUI
TEMPLATE  = app

# Tells bank_project.cpp to leave out its console main(); the GUI uses main.cpp.
DEFINES  += BANK_GUI

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    bank_project.cpp

HEADERS += \
    mainwindow.h \
    bank_backend.h

DISTFILES += \
    ../../source/repos/bank  1 project/bank  1 project/Bank.txt
