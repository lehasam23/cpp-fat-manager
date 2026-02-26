
QT += core gui widgets
CONFIG += c++11
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        src/custommodel.cpp \
        src/fatmodel.cpp \
        src/fatparser.cpp \
        src/leftpanel.cpp \
        src/main.cpp \
    src/mainwindow.cpp \
        src/rightpanel.cpp \
        src/toolbar.cpp \


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    include/custommodel.h \
    include/fatmodel.h \
    include/fatparser.h \
    include/leftpanel.h \
    include/mainwindow.h \
    include/rightpanel.h \
    include/toolbar.h \

RESOURCES += \
    resources.qrc

DISTFILES += \
    icons/about.png \
    icons/copy.png \
    icons/delete.png \
    icons/exit.png \
    icons/mkdir.png \
    icons/move.png \
    icons/size.png
