# octi

A GUI client for playing [OCTI](http://www.octi.net). The sources were last
touched as a Qt 2 / C++98 project in 2001; this tree now builds and runs on a
modern Linux machine with g++ and Qt 5.15.

## Build

Requires Qt 5 Widgets and PrintSupport (Ubuntu/Debian: `qtbase5-dev`,
`qt5-qmake`) and a C++14 compiler.

```sh
mkdir -p build
cd build
qmake ../build.pro
make -j$(nproc)
./octi
```

From the project root you can also run `make -C build` after the first `qmake`.

## What was ported

- Fixed 64-bit integer and endian types (the old headers treated `unsigned long`
  as 32-bit and assumed x86_64 was big-endian).
- Stopped redefining the C++ `true`/`false` keywords.
- Mapped old `iostream` names to `std::`.
- Replaced Qt 2 widgets (`QPopupMenu`, `QWorkspace`, `QTextDrag`, `QToolButton`
  constructors, Designer-generated dialogs, …) with Qt 5 equivalents.
- Restored the main window so a local two- or four-player game can be started.

The 2001 network client/server, trainer, and printer paths still compile as
stubs or incomplete features. Preferences and login dialogs open, but only the
local board is a real game again.
