#
# Makefile
#
# Copyright (C) 2026, Charles Chiou
#

LINUX_BUILD_DIR := build/linux
WINDOWS_BUILD_DIR := build/windows
CMAKE := $(shell if [ -x /usr/bin/cmake ]; then echo /usr/bin/cmake; else echo cmake; fi)
MINGW_CC := x86_64-w64-mingw32-gcc-posix
MINGW_CXX := x86_64-w64-mingw32-g++-posix
MINGW_RC := x86_64-w64-mingw32-windres
QT_PREBUILT_ROOT ?= $(shell if [ -d "$$HOME/work/aimon/build/qt-prebuilt" ]; then echo "$$HOME/work/aimon/build/qt-prebuilt"; else echo "$(CURDIR)/build/qt-prebuilt"; fi)

.PHONY: all linux windows clean distclean

all: linux windows

linux:
	@$(CMAKE) -S . -B $(LINUX_BUILD_DIR)
	@$(CMAKE) --build $(LINUX_BUILD_DIR) -j$$(nproc)

windows:
	@$(CMAKE) -S . -B $(WINDOWS_BUILD_DIR) \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER=$(MINGW_CC) \
		-DCMAKE_CXX_COMPILER=$(MINGW_CXX) \
		-DCMAKE_RC_COMPILER=$(MINGW_RC) \
		-DOCTI_QT_PREBUILT_ROOT="$(QT_PREBUILT_ROOT)"
	@$(CMAKE) --build $(WINDOWS_BUILD_DIR) -j$$(nproc)

clean:
	@rm -rf $(LINUX_BUILD_DIR) $(WINDOWS_BUILD_DIR)

distclean: clean
	@rm -rf build
