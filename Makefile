# Convenience wrapper. Prefer an out-of-tree qmake build in ./build
BUILDDIR := build

.PHONY: all run clean

all:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && qmake ../build.pro && $(MAKE)

run: all
	cd $(BUILDDIR) && ./octi

clean:
	$(MAKE) -C $(BUILDDIR) clean
