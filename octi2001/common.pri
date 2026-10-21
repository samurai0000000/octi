# Shared qmake settings for a modern g++ / Qt 5.15 build.
CONFIG += c++14
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CXXFLAGS += -Wno-write-strings
QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable
QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
QMAKE_CXXFLAGS += -Wno-unused-function
QMAKE_CXXFLAGS += -Wno-sign-compare
QMAKE_CXXFLAGS += -Wno-parentheses
QMAKE_CXXFLAGS += -Wno-reorder
QMAKE_CXXFLAGS += -fpermissive
unix: LIBS += -lpthread -ldl
