# octi.pro
#
# Copyright (C) 2022, Charles Chiou

QT += core gui widgets printsupport
TARGET = octi
TEMPLATE = app
include(common.pri)
INCLUDEPATH += ai/include game/include util/include include widget/include
LIBS += -L. -lss_ai -lss_game -lss_util
QMAKE_RPATHDIR += $$OUT_PWD
SOURCES += \
	src/Octi2PlayerGameSearchState.cxx \
	src/OctiAgent.cxx \
	src/OctiBoard.cxx \
	src/OctiBoardDispWidget.cxx \
	src/OctiBoardFrame.cxx \
	src/OctiGame.cxx \
	src/OctiGameFileHandler.cxx \
	src/OctiGameRandom.cxx \
	src/OctiGameSelectDialog.cxx \
	src/OctiGameSelectDialogBase.cxx \
	src/OctiMainWindow.cxx \
	src/OctiMove.cxx \
	src/OctiPod.cxx \
	src/OctiPreferencesDialog.cxx \
	src/OctiPreferencesDialogBase.cxx \
	src/OctiSquare.cxx \
	src/OctiSquareDispWidget.cxx \
	src/OctiStatDispWidget.cxx \
	src/OctiWidgetColors.cxx \
	src/OctiWidgetOptions.cxx \
	src/octiAppMain.cxx \
	widget/src/LoginDialog.cxx \
	widget/src/LoginDialogBase.cxx
HEADERS += \
	include/OctiBoardDispWidget.hxx \
	include/OctiBoardFrame.hxx \
	include/OctiGameSelectDialog.hxx \
	include/OctiGameSelectDialogBase.hxx \
	include/OctiMainWindow.hxx \
	include/OctiPreferencesDialog.hxx \
	include/OctiPreferencesDialogBase.hxx \
	include/OctiSquareDispWidget.hxx \
	include/OctiStatDispWidget.hxx \
	include/qt2compat.h \
	widget/include/LoginDialog.hxx \
	widget/include/LoginDialogBase.hxx
