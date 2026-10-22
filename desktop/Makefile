#
# Makefile
#
# Copyright (C) 2026, Charles Chiou
#

LINUX_BUILD_DIR := build/linux
WINDOWS_BUILD_DIR := build/windows
VERSION := $(shell sed -n 's/^.*\*\{2\}Version:[[:space:]]*\([0-9]\+\.[0-9]\+\.[0-9]\+\)\*\{2\}.*$$/\1/p' ../README.md)
WINDOWS_INSTALLER := $(WINDOWS_BUILD_DIR)/octi-setup-$(VERSION).exe
CMAKE := $(shell if [ -x /usr/bin/cmake ]; then echo /usr/bin/cmake; else echo cmake; fi)
MINGW_CC := x86_64-w64-mingw32-gcc-posix
MINGW_CXX := x86_64-w64-mingw32-g++-posix
MINGW_RC := x86_64-w64-mingw32-windres
QT_PREBUILT_ROOT ?= $(shell if [ -d "$$HOME/work/aimon/build/qt-prebuilt" ]; then echo "$$HOME/work/aimon/build/qt-prebuilt"; else echo "$(CURDIR)/build/qt-prebuilt"; fi)

SANITIZE ?= 0
ifneq (,$(findstring -fsanitize,$(CFLAGS) $(CXXFLAGS)))
SANITIZE := 1
endif
LINUX_CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Release -DOCTI_DESKTOP_SANITIZE=OFF
ifeq ($(SANITIZE),1)
LINUX_BUILD_DIR := build/linux-asan
LINUX_CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug -DOCTI_DESKTOP_SANITIZE=ON
endif

.PHONY: all linux windows installer test clean distclean

all: linux windows

linux:
	@if [ ! -f "$(LINUX_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) -S . -B $(LINUX_BUILD_DIR) $(LINUX_CMAKE_FLAGS); \
	fi
	@$(CMAKE) --build $(LINUX_BUILD_DIR) -j$$(nproc)

windows:
	@if [ ! -f "$(WINDOWS_BUILD_DIR)/CMakeCache.txt" ]; then \
		$(CMAKE) -S . -B $(WINDOWS_BUILD_DIR) \
			-DCMAKE_SYSTEM_NAME=Windows \
			-DCMAKE_C_COMPILER=$(MINGW_CC) \
			-DCMAKE_CXX_COMPILER=$(MINGW_CXX) \
			-DCMAKE_RC_COMPILER=$(MINGW_RC) \
			-DOCTI_QT_PREBUILT_ROOT="$(QT_PREBUILT_ROOT)"; \
	fi
	@$(CMAKE) --build $(WINDOWS_BUILD_DIR) -j$$(nproc)
	@test -f "$(WINDOWS_INSTALLER)"

installer: windows

test: linux windows
	@QT_QPA_PLATFORM=offscreen ./$(LINUX_BUILD_DIR)/test_desktop_board_and_composer
	@QT_QPA_PLATFORM=offscreen ./$(LINUX_BUILD_DIR)/test_desktop_network_and_voice
	@QT_QPA_PLATFORM=offscreen ./$(LINUX_BUILD_DIR)/test_desktop_app_integration

clean:
	@rm -rf build/linux build/linux-asan $(WINDOWS_BUILD_DIR)

distclean: clean
	@rm -rf build
