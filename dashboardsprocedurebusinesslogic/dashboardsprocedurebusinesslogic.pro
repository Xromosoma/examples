QT += gui sql core

greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets webenginewidgets
  DEFINES += HAVE_QT5 PL_USE_WEBENGINE

  win32|win64 {
    QT += axcontainer
    DEFINES += PL_USE_AX
  }

  android {
    CONFIG += mobility
  }
}

equals(QT_MAJOR_VERSION, 4) {
  win32|win64 {
    CONFIG  += qaxcontainer
    DEFINES += PL_USE_AX
  }

  !android {
    QT += webkit
  }
}

TARGET = ./../../../../plugins/business/modashboardsprocedurebusinesslogic
TEMPLATE = lib

CONFIG -= debug_and_release debug_and_release_target

include(./../business.pri)

DEFINES += MO_DASHBOARDSPROCEDUREBUSINESSLOGIC_LIBRARY

SOURCES += dashboardsprocedurebusinesslogic.cpp \
    dashboardwindow.cpp \
    dashboardsettingsdialog.cpp

HEADERS += dashboardsprocedurebusinesslogic.h \
    dashboardwindow.h \
    dashboardsettingsdialog.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

FORMS += \
    dashboardwindow.ui \
    dashboardsettingsdialog.ui
