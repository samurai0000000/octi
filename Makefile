##
## Makefile
##
## Copyright (C) 2026, Charles Chiou
##

SHELL := /bin/bash
VERSION := $(shell sed -n 's/^.*\*\{2\}Version:[[:space:]]*\([0-9]\+\.[0-9]\+\.[0-9]\+\)\*\{2\}.*$$/\1/p' README.md)
JAVA_HOME ?= $(shell ls -d "$$HOME"/.jdks/jdk-* 2>/dev/null | tail -n 1)
export JAVA_HOME

OCTIBOT_DIR ?= $(abspath $(CURDIR)/../octibot)

.PHONY: all core server desktop deb linux windows installer android test clean distclean sync-core-to-octibot sync-core-from-octibot

all: core server desktop android

core:
	@$(MAKE) -C core all

server:
	@$(MAKE) -C server all

deb:
	@$(MAKE) -C server deb

desktop:
	@$(MAKE) -C desktop all

linux:
	@$(MAKE) -C desktop linux

windows:
	@$(MAKE) -C desktop windows

installer:
	@$(MAKE) -C desktop installer

android:
	@if [ -d android ] && [ -x android/gradlew ]; then \
		cd android && ./gradlew assembleDebug; \
	fi

test:
	@$(MAKE) -C core test
	@$(MAKE) -C server test
	@$(MAKE) -C desktop test
	@if [ -d android ] && [ -x android/gradlew ]; then \
		cd android && ./gradlew testDebugUnitTest; \
	fi

clean:
	@$(MAKE) -C core clean
	@$(MAKE) -C server clean
	@$(MAKE) -C desktop clean
	@if [ -d android ] && [ -x android/gradlew ]; then \
		cd android && ./gradlew clean; \
	fi

distclean:
	@$(MAKE) -C core distclean
	@$(MAKE) -C server distclean
	@$(MAKE) -C desktop distclean
	@if [ -d android ] && [ -x android/gradlew ]; then \
		cd android && ./gradlew clean; \
	fi

sync-core-to-octibot:
	@if [ ! -d "$(OCTIBOT_DIR)" ]; then \
		echo "OCTIBOT_DIR $(OCTIBOT_DIR) does not exist"; exit 1; \
	fi
	@mkdir -p "$(OCTIBOT_DIR)/core"
	rsync -a --delete --exclude='build' core/ "$(OCTIBOT_DIR)/core/"
	@echo "Synced core/ to $(OCTIBOT_DIR)/core/"

sync-core-from-octibot:
	@if [ ! -d "$(OCTIBOT_DIR)/core" ]; then \
		echo "$(OCTIBOT_DIR)/core does not exist"; exit 1; \
	fi
	rsync -a --delete --exclude='build' "$(OCTIBOT_DIR)/core/" core/
	@echo "Synced core/ from $(OCTIBOT_DIR)/core/"
