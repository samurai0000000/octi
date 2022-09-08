# octi.pro
#
# Copyright (C) 2022, Charles Chiou

QT += core gui widgets
TARGET = octi
TEMPLATE = app
INCLUDEPATH += ai/include game/include util/include include
#LIBS += libss_ai.so libss_game.so libss_util.so
SOURCES += \
	src/fake.cxx
HEADERS +=
FORMS +=
