# Makefile
#
# Copyright (C) 2022, Charles Chiou

TARGETS = 	build/octi

.PHONY: default clean distclean

default: $(TARGETS)

clean:
	@[ -f build/Makefile ] && $(MAKE) --no-print-directory -C build clean
	@[ -d build ] && rm -f build/octi build/lib*.so*

distclean:
	@echo DISTCLEAN
	@rm -rf build

.PHONY: build/octi

build/octi: build/Makefile
	@$(MAKE) --no-print-directory -C build

build/Makefile: build.pro
	@mkdir -p build
	@cd build && qmake ../build.pro

