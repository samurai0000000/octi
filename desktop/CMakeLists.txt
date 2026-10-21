cmake_minimum_required(VERSION 3.16)

# Extract version from root README.md ("**Version: major.minor.patch**")
set(_ROOT_README_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_ROOT_README_FILE}")
file(READ "${_ROOT_README_FILE}" _ROOT_README_CONTENT)
string(REGEX MATCH "\\*\\*Version:[ \t]*([0-9]+)\\.([0-9]+)\\.([0-9]+)\\*\\*" _VER_MATCH "${_ROOT_README_CONTENT}")
if(NOT _VER_MATCH)
    message(FATAL_ERROR "Could not extract **Version: x.y.z** from ${_ROOT_README_FILE}")
endif()
set(PROJ_VER "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")

project(octi VERSION ${PROJ_VER} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter -UNDEBUG)
endif()

if(NOT TARGET octi_core)
    set(OCTI_CORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/core)
endif()

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchPrebuiltQtWindows.cmake)

    if(MINGW)
        # Match prebuilt Qt 6.8.3 MinGW-w64 libstdc++ ABI
        add_compile_options(-D_GLIBCXX_USE_INIT_PRIORITY_ATTRIBUTE=0)
        target_compile_options(octi_core PRIVATE -D_GLIBCXX_USE_INIT_PRIORITY_ATTRIBUTE=0)
    endif()

    add_executable(octi WIN32
        src/Main.cxx
    )

    target_include_directories(octi SYSTEM PRIVATE
        "${OCTI_QT_INCLUDE_DIR}"
        "${OCTI_QT_INCLUDE_DIR}/QtCore"
        "${OCTI_QT_INCLUDE_DIR}/QtGui"
        "${OCTI_QT_INCLUDE_DIR}/QtWidgets"
    )

    target_link_libraries(octi PRIVATE
        octi_core
        "${OCTI_QT_LIB_DIR}/libQt6Widgets.a"
        "${OCTI_QT_LIB_DIR}/libQt6Gui.a"
        "${OCTI_QT_LIB_DIR}/libQt6Core.a"
        ws2_32
        ole32
        shell32
        user32
        gdi32
        advapi32
    )
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

    add_executable(octi
        src/Main.cxx
    )

    target_link_libraries(octi PRIVATE
        octi_core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
    )
endif()
