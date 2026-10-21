#
# FetchPrebuiltQtWindows.cmake
#
# Copyright (C) 2026, Charles Chiou
#
# Retrieves prebuilt Windows Qt6 libraries via aqtinstall when cross-compiling
# for Windows (MinGW-w64). Never builds Qt from source.
#

set(OCTI_QT_WIN_VERSION "6.8.3" CACHE STRING "Prebuilt Windows Qt6 version")
set(OCTI_QT_WIN_ARCH "win64_mingw" CACHE STRING "Prebuilt Windows Qt6 arch")
set(OCTI_QT_WIN_SUBDIR "mingw_64" CACHE STRING "Prebuilt Windows Qt6 subdir")

if(NOT DEFINED OCTI_QT_PREBUILT_ROOT)
    set(OCTI_QT_PREBUILT_ROOT "${CMAKE_CURRENT_BINARY_DIR}/qt-prebuilt")
endif()

set(OCTI_QT_MINGW_ROOT "${OCTI_QT_PREBUILT_ROOT}/${OCTI_QT_WIN_VERSION}/${OCTI_QT_WIN_SUBDIR}")

if(NOT EXISTS "${OCTI_QT_MINGW_ROOT}/include/QtCore/qconfig.h")
    find_program(AQT_EXECUTABLE NAMES aqt PATHS "$ENV{HOME}/.local/bin")
    if(NOT AQT_EXECUTABLE)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        message(STATUS "Installing aqtinstall via pip...")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pip install --user aqtinstall
            RESULT_VARIABLE _pip_rc
        )
        if(NOT _pip_rc EQUAL 0)
            message(FATAL_ERROR "Failed to install aqtinstall via pip")
        endif()
        find_program(AQT_EXECUTABLE NAMES aqt PATHS "$ENV{HOME}/.local/bin" REQUIRED)
    endif()

    file(MAKE_DIRECTORY "${OCTI_QT_PREBUILT_ROOT}")
    message(STATUS "Downloading prebuilt Windows Qt ${OCTI_QT_WIN_VERSION} (${OCTI_QT_WIN_ARCH}) to ${OCTI_QT_PREBUILT_ROOT}...")
    execute_process(
        COMMAND "${AQT_EXECUTABLE}" install-qt windows desktop
                "${OCTI_QT_WIN_VERSION}" "${OCTI_QT_WIN_ARCH}"
                -O "${OCTI_QT_PREBUILT_ROOT}"
                --archives qtbase icu
        RESULT_VARIABLE _aqt_rc
    )
    if(NOT _aqt_rc EQUAL 0)
        message(FATAL_ERROR "aqt install-qt failed with exit code ${_aqt_rc}")
    endif()
endif()

set(OCTI_QT_INCLUDE_DIR "${OCTI_QT_MINGW_ROOT}/include")
set(OCTI_QT_LIB_DIR "${OCTI_QT_MINGW_ROOT}/lib")
set(OCTI_QT_BIN_DIR "${OCTI_QT_MINGW_ROOT}/bin")
set(OCTI_QT_PLUGINS_DIR "${OCTI_QT_MINGW_ROOT}/plugins")

message(STATUS "Using prebuilt Windows Qt6 at: ${OCTI_QT_MINGW_ROOT}")
