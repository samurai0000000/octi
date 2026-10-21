# game.pro
#
# Copyright (C) 2022, Charles Chiou

QT -= gui
CONFIG += shared
TARGET = ../ss_game
TEMPLATE = lib
include(../common.pri)
INCLUDEPATH += include ../util/include
LIBS += -L.. -lss_util
SOURCES += \
	src/GameClient.cxx \
	src/GameExceptions.cxx \
	src/GameServer.cxx \
	src/GameSession.cxx


