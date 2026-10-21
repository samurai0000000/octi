# ai.pro
#
# Copyright (C) 2022, Charles Chiou

QT -= gui
CONFIG += shared
TARGET = ../ss_ai
TEMPLATE = lib
include(../common.pri)
INCLUDEPATH += include ../util/include
LIBS += -L.. -lss_util
SOURCES += \
	src/AISearchAlgorithm.cxx \
	src/AlphaBetaSearch.cxx \
	src/ERSearch.cxx \
	src/MPIAlphaBetaSearch.cxx \
	src/ThreadAlphaBetaSearch.cxx \
	src/TranspositionTable.cxx
