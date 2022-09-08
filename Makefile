# Makefile
#
# Copyright (C) 2022, Charles Chiou

TARGETS = 	build/octi

DEP_LIBS =	build/libss_ai.so build/libss_game.so build/libss_util.so

.PHONY: default clean distclean

default: $(TARGETS)

clean:
	@rm -f $(DEP_LIBS)
	@for d in build/ai build/game build/util; do \
		[ -f $$d/Makefile ] && \
		$(MAKE) --no-print-directory -C $$d clean; \
	done

distclean:
	@echo DISTCLEAN
	@rm -rf build

build/octi: $(DEP_LIBS)

AI_DEPS :=	$(shell find ai -type f)

build/libss_ai.so: build/ai/libss_ai.so
	install -m 755 $< $@

build/ai/libss_ai.so: build/ai/Makefile $(AI_DEPS)
	@$(MAKE) --no-print-directory -C build/ai

build/ai/Makefile: ai/CMakeLists.txt
	@mkdir -p build/ai
	@cd build/ai && cmake -Wno-dev ../../ai

GAME_DEPS :=	$(shell find game -type f)

build/libss_game.so: build/game/libss_game.so
	install -m 755 $< $@

build/game/libss_game.so: build/game/Makefile $(GAME_DEPS)
	@$(MAKE) --no-print-directory -C build/game

build/game/Makefile: game/CMakeLists.txt
	@mkdir -p build/game
	@cd build/game && cmake -Wno-dev ../../game

UTIL_DEPS :=	$(shell find util -type f)

build/libss_util.so: build/util/libss_util.so
	install -m 755 $< $@

build/util/libss_util.so: build/util/Makefile $(UTIL_DEPS)
	@$(MAKE) --no-print-directory -C build/util

build/util/Makefile: util/CMakeLists.txt
	@mkdir -p build/util
	@cd build/util && cmake -Wno-dev ../../util
