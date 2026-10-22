cmake_minimum_required(VERSION 3.16)

# Extract version from root README.md ("**Version: major.minor.patch**")
set(_ROOT_README_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_ROOT_README_FILE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/octi.nsi.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/octi.rc.in"
)
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

option(OCTI_DESKTOP_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
if(WIN32)
    option(OCTI_DESKTOP_BUILD_TESTS "Build octi_desktop CppUTest suite" OFF)
else()
    option(OCTI_DESKTOP_BUILD_TESTS "Build octi_desktop CppUTest suite" ON)
endif()

if(OCTI_DESKTOP_SANITIZE AND NOT MSVC AND NOT WIN32)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer
                        -fno-sanitize-recover=undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

if(NOT TARGET octi_core)
    set(OCTI_CORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(OCTI_CORE_SANITIZE ${OCTI_DESKTOP_SANITIZE} CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/core)
endif()

set(OCTI_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")
if(NOT EXISTS "${OCTI_THIRD_PARTY_DIR}/cpp-httplib/httplib.h" OR
   NOT EXISTS "${OCTI_THIRD_PARTY_DIR}/json/include/nlohmann/json.hpp")
    message(FATAL_ERROR
        "third_party submodules missing; run: git submodule update --init --recursive")
endif()

set(OCTI_DESKTOP_LIB_SOURCES
    src/OctiTheme.cxx
    src/OctiBoardGeometry.cxx
    src/OctiMoveComposer.cxx
    src/OctiSoundSynth.cxx
    src/OctiBoardCanvas.cxx
    src/OctiReservePanel.cxx
    src/ClientConfigStore.cxx
    src/OctiNetworkClient.cxx
    src/P2pVoiceEngine.cxx
    src/OctiDialogs.cxx
    src/LobbyView.cxx
    src/OctiMainWindow.cxx
)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchPrebuiltQtWindows.cmake)

    if(MINGW)
        # Match prebuilt Qt 6.8.3 MinGW-w64 libstdc++ ABI
        add_compile_options(-D_GLIBCXX_USE_INIT_PRIORITY_ATTRIBUTE=0)
        target_compile_options(octi_core PRIVATE -D_GLIBCXX_USE_INIT_PRIORITY_ATTRIBUTE=0)
    endif()

    add_library(octi_desktop_lib STATIC
        ${OCTI_DESKTOP_LIB_SOURCES}
    )

    target_include_directories(octi_desktop_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(octi_desktop_lib SYSTEM PUBLIC
        ${OCTI_THIRD_PARTY_DIR}/cpp-httplib
        ${OCTI_THIRD_PARTY_DIR}/json/include
        "${OCTI_QT_INCLUDE_DIR}"
        "${OCTI_QT_INCLUDE_DIR}/QtCore"
        "${OCTI_QT_INCLUDE_DIR}/QtGui"
        "${OCTI_QT_INCLUDE_DIR}/QtWidgets"
    )

    target_link_libraries(octi_desktop_lib PUBLIC
        octi_core
        "${OCTI_QT_LIB_DIR}/libQt6Widgets.a"
        "${OCTI_QT_LIB_DIR}/libQt6Gui.a"
        "${OCTI_QT_LIB_DIR}/libQt6Core.a"
        ws2_32
        crypt32
        winmm
        ole32
        shell32
        user32
        gdi32
        advapi32
    )

    if(MINGW)
        if(NOT CMAKE_RC_COMPILER)
            set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
        endif()
    endif()
    enable_language(RC)

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/octi.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/octi.rc"
        @ONLY
    )

    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/octi.rc"
        PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/octi.ico"
    )

    add_executable(octi WIN32
        src/Main.cxx
        "${CMAKE_CURRENT_BINARY_DIR}/octi.rc"
    )

    target_link_libraries(octi PRIVATE
        octi_desktop_lib
    )

    set(OCTI_WIN_QT_DLLS
        Qt6Core.dll
        Qt6Gui.dll
        Qt6Widgets.dll
    )
    set(OCTI_WIN_MINGW_DLLS
        libstdc++-6.dll
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
    )
    set(OCTI_WIN_PLATFORM_PLUGINS
        qwindows.dll
        qdirect2d.dll
        qminimal.dll
        qoffscreen.dll
    )
    set(OCTI_WIN_STYLE_PLUGINS)
    if(EXISTS "${OCTI_QT_PLUGINS_DIR}/styles/qmodernwindowsstyle.dll")
        list(APPEND OCTI_WIN_STYLE_PLUGINS qmodernwindowsstyle.dll)
    endif()

    set(OCTI_STAGED_FILES)
    set(OCTI_STAGE_COMMANDS
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/platforms"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/styles"
    )
    foreach(_dll IN LISTS OCTI_WIN_QT_DLLS)
        list(APPEND OCTI_STAGED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${_dll}")
        list(APPEND OCTI_STAGE_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${OCTI_QT_BIN_DIR}/${_dll}"
                "${CMAKE_CURRENT_BINARY_DIR}/${_dll}"
        )
    endforeach()
    foreach(_dll IN LISTS OCTI_WIN_MINGW_DLLS)
        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${_dll}"
            OUTPUT_VARIABLE _MINGW_DLL_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT EXISTS "${_MINGW_DLL_PATH}")
            set(_MINGW_DLL_PATH "${OCTI_QT_BIN_DIR}/${_dll}")
        endif()
        list(APPEND OCTI_STAGED_FILES "${CMAKE_CURRENT_BINARY_DIR}/${_dll}")
        list(APPEND OCTI_STAGE_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_MINGW_DLL_PATH}"
                "${CMAKE_CURRENT_BINARY_DIR}/${_dll}"
        )
    endforeach()
    foreach(_plug IN LISTS OCTI_WIN_PLATFORM_PLUGINS)
        list(APPEND OCTI_STAGED_FILES "${CMAKE_CURRENT_BINARY_DIR}/platforms/${_plug}")
        list(APPEND OCTI_STAGE_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${OCTI_QT_PLUGINS_DIR}/platforms/${_plug}"
                "${CMAKE_CURRENT_BINARY_DIR}/platforms/${_plug}"
        )
    endforeach()
    foreach(_style IN LISTS OCTI_WIN_STYLE_PLUGINS)
        list(APPEND OCTI_STAGED_FILES "${CMAKE_CURRENT_BINARY_DIR}/styles/${_style}")
        list(APPEND OCTI_STAGE_COMMANDS
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${OCTI_QT_PLUGINS_DIR}/styles/${_style}"
                "${CMAKE_CURRENT_BINARY_DIR}/styles/${_style}"
        )
    endforeach()

    add_custom_command(TARGET octi POST_BUILD
        ${OCTI_STAGE_COMMANDS}
        COMMENT "Staging Qt6 and MinGW runtime DLLs and plugins alongside octi.exe"
    )

    find_program(MAKENSIS_EXECUTABLE NAMES makensis NO_CMAKE_FIND_ROOT_PATH REQUIRED)

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/octi.nsi.in"
        "${CMAKE_CURRENT_BINARY_DIR}/octi.nsi"
        @ONLY
    )

    set(OCTI_WINDOWS_INSTALLER_EXE
        "${CMAKE_CURRENT_BINARY_DIR}/octi-setup-${PROJECT_VERSION}.exe"
    )

    add_custom_command(
        OUTPUT "${OCTI_WINDOWS_INSTALLER_EXE}" ${OCTI_STAGED_FILES}
        ${OCTI_STAGE_COMMANDS}
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${OCTI_WINDOWS_INSTALLER_EXE}"
        COMMAND "${MAKENSIS_EXECUTABLE}" -V2 "${CMAKE_CURRENT_BINARY_DIR}/octi.nsi"
        DEPENDS
            octi
            "${CMAKE_CURRENT_BINARY_DIR}/octi.nsi"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/octi.ico"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/nsis_welcome.bmp"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/nsis_header.bmp"
            "${_ROOT_README_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Building Windows NSIS installer octi-setup-${PROJECT_VERSION}.exe"
        VERBATIM
    )

    add_custom_target(octi_windows_installer ALL
        DEPENDS "${OCTI_WINDOWS_INSTALLER_EXE}"
    )
else()
    find_package(Threads REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_library(ALSA_LIBRARY asound REQUIRED)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

    add_library(octi_desktop_lib STATIC
        ${OCTI_DESKTOP_LIB_SOURCES}
    )

    target_include_directories(octi_desktop_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(octi_desktop_lib SYSTEM PUBLIC
        ${OCTI_THIRD_PARTY_DIR}/cpp-httplib
        ${OCTI_THIRD_PARTY_DIR}/json/include
    )

    target_compile_definitions(octi_desktop_lib PUBLIC
        CPPHTTPLIB_OPENSSL_SUPPORT
    )

    target_link_libraries(octi_desktop_lib PUBLIC
        octi_core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        ${ALSA_LIBRARY}
    )

    add_executable(octi
        src/Main.cxx
    )

    target_link_libraries(octi PRIVATE
        octi_desktop_lib
    )

    if(OCTI_DESKTOP_BUILD_TESTS)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(CPPUTEST REQUIRED cpputest)
        find_library(RAPIDCHECK_LIBRARY rapidcheck REQUIRED)

        if(NOT TARGET octi_server_lib)
            set(OCTI_SERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(OCTI_SERVER_SANITIZE ${OCTI_DESKTOP_SANITIZE} CACHE BOOL "" FORCE)
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../server ${CMAKE_CURRENT_BINARY_DIR}/server)
        endif()

        add_executable(test_desktop_board_and_composer
            test/TestDesktopBoardAndComposer.cxx
        )

        target_include_directories(test_desktop_board_and_composer PRIVATE
            ${CPPUTEST_INCLUDE_DIRS}
        )

        target_link_directories(test_desktop_board_and_composer PRIVATE
            ${CPPUTEST_LIBRARY_DIRS}
        )

        target_link_libraries(test_desktop_board_and_composer PRIVATE
            octi_desktop_lib
            octi_core
            ${CPPUTEST_LIBRARIES}
            ${RAPIDCHECK_LIBRARY}
        )

        target_compile_definitions(test_desktop_board_and_composer PRIVATE
            OCTI_REPO_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
        )

        add_executable(test_desktop_network_and_voice
            test/TestDesktopNetworkAndVoice.cxx
        )

        target_include_directories(test_desktop_network_and_voice PRIVATE
            ${CPPUTEST_INCLUDE_DIRS}
        )

        target_link_directories(test_desktop_network_and_voice PRIVATE
            ${CPPUTEST_LIBRARY_DIRS}
        )

        target_link_libraries(test_desktop_network_and_voice PRIVATE
            octi_desktop_lib
            octi_server_lib
            octi_core
            ${CPPUTEST_LIBRARIES}
            ${RAPIDCHECK_LIBRARY}
        )

        target_compile_definitions(test_desktop_network_and_voice PRIVATE
            OCTI_DESKTOP_HERMETIC_TEST=1
            OCTI_REPO_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
        )

        add_executable(test_desktop_app_integration
            test/TestDesktopAppIntegration.cxx
        )

        target_include_directories(test_desktop_app_integration PRIVATE
            ${CPPUTEST_INCLUDE_DIRS}
        )

        target_link_directories(test_desktop_app_integration PRIVATE
            ${CPPUTEST_LIBRARY_DIRS}
        )

        target_link_libraries(test_desktop_app_integration PRIVATE
            octi_desktop_lib
            octi_server_lib
            octi_core
            ${CPPUTEST_LIBRARIES}
            ${RAPIDCHECK_LIBRARY}
        )

        target_compile_definitions(test_desktop_app_integration PRIVATE
            OCTI_DESKTOP_HERMETIC_TEST=1
            OCTI_REPO_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
        )
    endif()
endif()
