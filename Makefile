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

BOOK_DIR ?= $(abspath $(CURDIR)/.book)
BOOK_MODE ?= all
BOOK_MAX_PLY ?= 6
BOOK_DEPTH ?= 9
BOOK_BOT_BRANCH ?= 3
BOOK_OPP_BRANCH ?= 6

CMAKE_CMD := $(shell if [ -x /usr/bin/cmake ]; then echo /usr/bin/cmake; else echo cmake; fi)

.PHONY: all core server desktop deb linux windows installer android test clean distclean sync-core-to-octibot sync-core-from-octibot opening-book opening-book-sync opening-book-cpp opening-book-java opening-book-js

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

opening-book:
	@if [ -n "$(BOOK_THREADS)" ]; then \
		THREADS="$(BOOK_THREADS)"; \
	elif [[ " $$MAKEFLAGS " =~ [[:space:]]-?j[[:space:]]*([0-9]+) ]]; then \
		THREADS="$${BASH_REMATCH[1]}"; \
	elif [[ " $$MAKEFLAGS " =~ [[:space:]]-?j([[:space:]]|$$) ]]; then \
		THREADS="$$(nproc 2>/dev/null || echo 4)"; \
	else \
		THREADS=1; \
	fi; \
	mkdir -p core/build && \
	(cd core/build && (test -f Makefile || $(CMAKE_CMD) $(CURDIR)/core -DCMAKE_BUILD_TYPE=Release)) && \
	MAKEFLAGS= $(MAKE) --no-print-directory -C $(CURDIR)/core/build octi_book_gen -j"$$THREADS" && \
	mkdir -p "$(BOOK_DIR)" && \
	cp -f core/build/octi_book_gen "$(BOOK_DIR)/octi_book_gen" && \
	"$(BOOK_DIR)/octi_book_gen" \
		--mode "$(BOOK_MODE)" \
		--max-ply "$(BOOK_MAX_PLY)" \
		--search-depth "$(BOOK_DEPTH)" \
		--bot-branch "$(BOOK_BOT_BRANCH)" \
		--opp-branch "$(BOOK_OPP_BRANCH)" \
		--threads "$$THREADS" \
		--out-json "$(BOOK_DIR)/opening_book.json" \
		--out-header "$(BOOK_DIR)/OctiOpeningBookData.hxx" \
		--out-java "$(BOOK_DIR)/OctiOpeningBookData.java" && \
	MAKEFLAGS= $(MAKE) --no-print-directory opening-book-sync BOOK_DIR="$(BOOK_DIR)"

opening-book-cpp:
	@if [ ! -x "$(BOOK_DIR)/octi_book_gen" ]; then \
		mkdir -p core/build && \
		(cd core/build && (test -f Makefile || $(CMAKE_CMD) $(CURDIR)/core -DCMAKE_BUILD_TYPE=Release)) && \
		MAKEFLAGS= $(MAKE) --no-print-directory -C $(CURDIR)/core/build octi_book_gen -j"$$(nproc 2>/dev/null || echo 4)" && \
		mkdir -p "$(BOOK_DIR)" && \
		cp -f core/build/octi_book_gen "$(BOOK_DIR)/octi_book_gen"; \
	fi
	@if [ ! -f "$(BOOK_DIR)/opening_book.json" ]; then \
		echo "Error: $(BOOK_DIR)/opening_book.json not found. Run 'make opening-book' first."; exit 1; \
	fi
	@"$(BOOK_DIR)/octi_book_gen" \
		--from-json "$(BOOK_DIR)/opening_book.json" \
		--out-header core/include/OctiOpeningBookData.hxx

opening-book-java:
	@if [ ! -x "$(BOOK_DIR)/octi_book_gen" ]; then \
		mkdir -p core/build && \
		(cd core/build && (test -f Makefile || $(CMAKE_CMD) $(CURDIR)/core -DCMAKE_BUILD_TYPE=Release)) && \
		MAKEFLAGS= $(MAKE) --no-print-directory -C $(CURDIR)/core/build octi_book_gen -j"$$(nproc 2>/dev/null || echo 4)" && \
		mkdir -p "$(BOOK_DIR)" && \
		cp -f core/build/octi_book_gen "$(BOOK_DIR)/octi_book_gen"; \
	fi
	@if [ ! -f "$(BOOK_DIR)/opening_book.json" ]; then \
		echo "Error: $(BOOK_DIR)/opening_book.json not found. Run 'make opening-book' first."; exit 1; \
	fi
	@"$(BOOK_DIR)/octi_book_gen" \
		--from-json "$(BOOK_DIR)/opening_book.json" \
		--out-java android/app/src/main/java/com/selfso/octi/OctiOpeningBookData.java

opening-book-js:
	@if [ ! -x "$(BOOK_DIR)/octi_book_gen" ]; then \
		mkdir -p core/build && \
		(cd core/build && (test -f Makefile || $(CMAKE_CMD) $(CURDIR)/core -DCMAKE_BUILD_TYPE=Release)) && \
		MAKEFLAGS= $(MAKE) --no-print-directory -C $(CURDIR)/core/build octi_book_gen -j"$$(nproc 2>/dev/null || echo 4)" && \
		mkdir -p "$(BOOK_DIR)" && \
		cp -f core/build/octi_book_gen "$(BOOK_DIR)/octi_book_gen"; \
	fi
	@if [ ! -f "$(BOOK_DIR)/opening_book.json" ]; then \
		echo "Error: $(BOOK_DIR)/opening_book.json not found. Run 'make opening-book' first."; exit 1; \
	fi
	@"$(BOOK_DIR)/octi_book_gen" \
		--from-json "$(BOOK_DIR)/opening_book.json" \
		--out-js server/src/WebUi.cxx \
		--out-js ../www/octi/html/index.html

opening-book-sync:
	@if [ ! -x "$(BOOK_DIR)/octi_book_gen" ]; then \
		mkdir -p core/build && \
		(cd core/build && (test -f Makefile || $(CMAKE_CMD) $(CURDIR)/core -DCMAKE_BUILD_TYPE=Release)) && \
		MAKEFLAGS= $(MAKE) --no-print-directory -C $(CURDIR)/core/build octi_book_gen -j"$$(nproc 2>/dev/null || echo 4)" && \
		mkdir -p "$(BOOK_DIR)" && \
		cp -f core/build/octi_book_gen "$(BOOK_DIR)/octi_book_gen"; \
	fi
	@if [ ! -f "$(BOOK_DIR)/opening_book.json" ]; then \
		echo "Error: $(BOOK_DIR)/opening_book.json not found. Run 'make opening-book' first."; exit 1; \
	fi
	@"$(BOOK_DIR)/octi_book_gen" \
		--from-json "$(BOOK_DIR)/opening_book.json" \
		--out-header core/include/OctiOpeningBookData.hxx \
		--out-java android/app/src/main/java/com/selfso/octi/OctiOpeningBookData.java \
		--out-js server/src/WebUi.cxx \
		--out-js ../www/octi/html/index.html
