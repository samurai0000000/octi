##
## Makefile
##
## Copyright (C) 2026, Charles Chiou
##

SHELL := /bin/bash
VERSION := $(shell sed -n 's/^.*\*\{2\}Version:[[:space:]]*\([0-9]\+\.[0-9]\+\.[0-9]\+\)\*\{2\}.*$$/\1/p' README.md)
JAVA_HOME ?= $(shell ls -d "$$HOME"/.jdks/jdk-* 2>/dev/null | tail -n 1)
export JAVA_HOME

.PHONY: all core server desktop linux windows installer android test clean distclean

all: core server desktop android

core:
	@$(MAKE) -C core all

server:
	@$(MAKE) -C server all

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
