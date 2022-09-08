# ai.pro
#
# Copyright (C) 2022, Charles Chiou

QT +=
CONFIG += dll
TARGET = ../ss_ai
TEMPLATE = lib
INCLUDEPATH += include ../util/include
SOURCES += \
	src/AISearchAlgorithm.cxx \
	src/AlphaBetaSearch.cxx \
	src/ERSearch.cxx \
	src/MPIAlphaBetaSearch.cxx \
	src/ThreadAlphaBetaSearch.cxx \
	src/TranspositionTable.cxx
