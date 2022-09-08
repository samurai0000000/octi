# game.pro
#
# Copyright (C) 2022, Charles Chiou

QT +=
CONFIG += dll
TARGET = ../ss_game
TEMPLATE = lib
INCLUDEPATH += include ../util/include
SOURCES += \
	src/GameClient.cxx \
	src/GameExceptions.cxx \
	src/GameServer.cxx \
	src/GameSession.cxx


