# util.pro
#
# Copyright (C) 2022, Charles Chiou

QT -= gui
CONFIG += shared
TARGET = ../ss_util
TEMPLATE = lib
include(../common.pri)
INCLUDEPATH += include
SOURCES += \
	src/Adler32.cxx \
	src/argproc.c \
	src/ArrayException.cxx \
	src/Barrier.cxx \
	src/Base64.cxx \
	src/BaseObject.cxx \
	src/BigInteger.cxx \
	src/Boolean.cxx \
	src/BufferedArray.cxx \
	src/ByteUtil.cxx \
	src/Calendar.cxx \
	src/Checksum.cxx \
	src/Collection.cxx \
	src/Condition.cxx \
	src/Config.cxx \
	src/CRC32.cxx \
	src/Date.cxx \
	src/debug.c \
	src/Dlist.cxx \
	src/DllManager.cxx \
	src/Error.cxx \
	src/Event.cxx \
	src/Exception.cxx \
	src/File.cxx \
	src/Graph.cxx \
	src/HashTable.cxx \
	src/Hex.cxx \
	src/Inet.cxx \
	src/IOException.cxx \
	src/Main.cxx \
	src/Matrix.cxx \
	src/Monitor.cxx \
	src/Mutex.cxx \
	src/Packet.cxx \
	src/Queue.cxx \
	src/Slist.cxx \
	src/Socket.cxx \
	src/Stack.cxx \
	src/String.cxx \
	src/StringTokenizer.cxx \
	src/Thread.cxx \
	src/ThreadException.cxx \
	src/ThreadWorkerPool.cxx \
	src/Timer.cxx \
	src/TimeZone.cxx \
	src/Tree.cxx \
	src/UU.cxx \
	src/Void.cxx \
	src/WorkerPool.cxx
HEADERS += include/basecfg.h
