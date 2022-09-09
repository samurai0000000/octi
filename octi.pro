# octi.pro
#
# Copyright (C) 2022, Charles Chiou

QT += core gui widgets
TARGET = octi
TEMPLATE = app
INCLUDEPATH += ai/include game/include util/include include
LIBS += -L. -lss_ai -lss_game -lss_util -ldl
SOURCES += \
	src/Octi2PlayerGameSearchState.cxx \
	src/OctiAgent.cxx \
	src/OctiBoard.cxx \
	src/OctiGame.cxx \
	src/OctiGameFileHandler.cxx \
	src/OctiGameRandom.cxx \
	src/OctiMove.cxx \
	src/OctiPod.cxx \
	src/OctiSquare.cxx \
	src/OctiWidgetColors.cxx \
	src/OctiWidgetOptions.cxx \
	src/octiAppMain.cxx
HEADERS +=
FORMS +=
