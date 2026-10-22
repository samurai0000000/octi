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

project(octi_server VERSION ${PROJ_VER} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter -UNDEBUG)
endif()

option(OCTI_SERVER_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
if(OCTI_SERVER_SANITIZE AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer
                        -fno-sanitize-recover=undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

if(NOT TARGET octi_core)
    set(OCTI_CORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(OCTI_CORE_SANITIZE ${OCTI_SERVER_SANITIZE} CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/core)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CPPUTEST REQUIRED cpputest)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_library(RAPIDCHECK_LIBRARY rapidcheck REQUIRED)

# Third-party header-only libraries are git submodules (never loose files).
set(OCTI_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")
if(NOT EXISTS "${OCTI_THIRD_PARTY_DIR}/cpp-httplib/httplib.h" OR
   NOT EXISTS "${OCTI_THIRD_PARTY_DIR}/json/include/nlohmann/json.hpp")
    message(FATAL_ERROR
        "third_party submodules missing; run: git submodule update --init --recursive")
endif()

add_library(octi_server_lib STATIC
    src/ServerApp.cxx
    src/OidcVerifier.cxx
    src/AccountStore.cxx
)

target_include_directories(octi_server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(octi_server_lib SYSTEM PUBLIC
    ${OCTI_THIRD_PARTY_DIR}/cpp-httplib
    ${OCTI_THIRD_PARTY_DIR}/json/include
)

target_compile_definitions(octi_server_lib PUBLIC
    CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(octi_server_lib PUBLIC
    octi_core
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
)

add_executable(octi_server
    src/Main.cxx
)

target_link_libraries(octi_server PRIVATE
    octi_server_lib
)

add_executable(test_server_foundation
    test/TestServerFoundation.cxx
)

target_include_directories(test_server_foundation PRIVATE
    ${CPPUTEST_INCLUDE_DIRS}
)

target_link_directories(test_server_foundation PRIVATE
    ${CPPUTEST_LIBRARY_DIRS}
)

target_link_libraries(test_server_foundation PRIVATE
    octi_server_lib
    ${CPPUTEST_LIBRARIES}
)

add_executable(test_account_and_oidc
    test/TestAccountAndOidc.cxx
)

target_include_directories(test_account_and_oidc PRIVATE
    ${CPPUTEST_INCLUDE_DIRS}
)

target_link_directories(test_account_and_oidc PRIVATE
    ${CPPUTEST_LIBRARY_DIRS}
)

target_link_libraries(test_account_and_oidc PRIVATE
    octi_server_lib
    ${CPPUTEST_LIBRARIES}
    ${RAPIDCHECK_LIBRARY}
)
