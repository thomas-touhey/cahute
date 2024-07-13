cmake_minimum_required(VERSION 3.16)

project(
    cahute
    VERSION 0.4
    DESCRIPTION "Communication and file format handling tools for CASIO calculators"
    HOMEPAGE_URL "https://cahuteproject.org/"
    LANGUAGES C
)
set(PROJECT_ISSUES_URL "https://cahuteproject.org/guides/report.html")

# ---
# Options.
# ---

include(CMakeDependentOption)

cmake_dependent_option(ENABLE_CLI "Whether to enable building the utilities or not."
    ON "NOT DEFINED FXSDK_PLATFORM" OFF)
cmake_dependent_option(ENABLE_SDL "Whether to enable building SDL2 dependent utilities or not."
    ON "NOT DEFINED AMIGA" OFF)
cmake_dependent_option(ENABLE_UDEV "Whether to install the udev rule or not."
    ON "NOT DEFINED FXSDK_PLATFORM AND NOT DEFINED AMIGA" OFF)
cmake_dependent_option(ENABLE_LIBUSB "Whether to use libusb or not."
    ON "NOT DEFINED FXSDK_PLATFORM AND NOT DEFINED AMIGA" OFF)
set(LOGLEVEL $ENV{LOGLEVEL} CACHE STRING "Default logging level")

# TODO: CaS is in its very early stages and supports too few features
# as of today to be useful installed on a target system. Once this is no
# longer the case, this option will be ineffective.
option(ENABLE_INCOMPLETE_CAS "Whether to build incomplete CaS or not." OFF)

if(NOT "${LOGLEVEL}" MATCHES "|none|info|warning|error|fatal")
    message(SEND_ERROR "LOGLEVEL should be one of [info, warning, error, fatal, none]")
endif()

if(NOT LOGLEVEL)
    set(LOGLEVEL "fatal")
endif()

# ---
# Other options.
# ---

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(
    CPACK_SOURCE_IGNORE_FILES
    "\\.git" "/build/" ".*~$" "\\.envrc" "\\.pre-commit-config\\.yaml"
    "\\.tool-versions" "\\.clang-format" "__pycache__" "\\.gitlab-ci.yml"
    "_CPack" "/docs/"
)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_VERBATIM_VARIABLES true)
include(CPack)

string(TOUPPER ${LOGLEVEL} DEFAULT_LOGLEVEL)

execute_process(
  COMMAND printf "0x%02X%02X0000" ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR}
  OUTPUT_VARIABLE PROJECT_VERSION_HEX
)

configure_file(include/cahute/config.h.in include/cahute/config.h ESCAPE_QUOTES)

add_custom_target(lint COMMAND pre-commit run --all-files)

# ---
# Build options.
# ---
# Here, for portability, we define the following variables:
#
#   LIB_INCLUDE_DIRS
#      Include directories for the library.
#
#   LIB_COMPILE_DEFS
#      Compile definitions for the library.
#
#   CLI_INCLUDE_DIRS
#      Include directories for the command-line utilities.
#
#   CLI_LIBRARIES
#      Additional libraries for the command-line utilities.
#
#   PKGCONFIG_DEPS
#      pkg-config additional dependencies.

set(LIB_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set(CLI_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
set(CLI_LIBRARIES
    ${PROJECT_NAME}
)
set(PKGCONFIG_DEPS "")
set(LIB_COMPILE_DEFS "")

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(Python3 REQUIRED)

if(WIN32 OR MINGW)
    list(APPEND CLI_LIBRARIES cfgmgr32 uuid)
    set(SDL2_NO_MWINDOWS 1)
endif()

if(ENABLE_CLI AND ENABLE_SDL)
    find_package(SDL2 REQUIRED)
endif()

if(ENABLE_LIBUSB)
    pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)
    list(APPEND LIB_INCLUDE_DIRS
        "${libusb_INCLUDE_DIRS}"
    )
    list(APPEND LIB_COMPILE_DEFS
        "-DLIBUSB_ENABLED=1"
    )
    list(APPEND CLI_LIBRARIES
        PkgConfig::libusb
    )
    list(APPEND PKGCONFIG_DEPS
        "libusb-1.0"
    )
else()
    list(APPEND LIB_COMPILE_DEFS
        "-DLIBUSB_ENABLED=0"
    )
endif()

set(CMAKE_C_STANDARD 90)
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Wno-c99-extensions -Wc23-extensions
    )
else()
    add_compile_options(
        -Wall -Wextra -Wno-unused-macros -Wno-vla
        -Wshadow -Wwrite-strings -Wredundant-decls -Wformat -Wformat-nonliteral
        -Wformat-security -Wimplicit-function-declaration
        -Wdate-time -Wmissing-prototypes -Wreturn-type -Wpointer-arith
        -Wstack-protector -Wno-unused-parameter
    )
endif()

# ---
# Target definitions.
# ---

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
    COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/misc/process_bin.py
        "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
        "${CMAKE_SOURCE_DIR}/cli/fxremote-update.exe.bin"
    DEPENDS cli/fxremote-update.exe.bin misc/process_bin.py
    VERBATIM
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
    COMMAND "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/chars/process_chars.py
        "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
    DEPENDS chars/process_chars.py chars/chars.toml
    VERBATIM
)

add_library(${PROJECT_NAME} STATIC
    lib/casiolink.c
    lib/cdefs.c
    "${CMAKE_CURRENT_BINARY_DIR}/lib/chars.c"
    lib/data.c
    lib/detection.c
    lib/link.c
    lib/linkopen.c
    lib/logging.c
    lib/mcs.c
    lib/medium.c
    lib/misc.c
    lib/picture.c
    lib/seven.c
    lib/seven_ohp.c
    lib/text.c
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIB_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/lib")
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LIB_COMPILE_DEFS})

if(ENABLE_CLI)
    if(ENABLE_INCOMPLETE_CAS)
        add_executable(CaS
            cli/cas.c
            cli/cas_args.c
            cli/casrc.c
            cli/common.c
            cli/options.c
        )
        target_link_libraries(CaS PRIVATE ${CLI_LIBRARIES})
        target_include_directories(CaS PRIVATE ${CLI_INCLUDE_DIRS})
    endif()

    add_executable(p7
        cli/p7.c
        cli/p7_args.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(p7 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7 PRIVATE ${CLI_INCLUDE_DIRS})

    add_executable(p7os
        cli/p7os.c
        cli/p7os_args.c
        cli/common.c
        cli/options.c
        "${CMAKE_CURRENT_BINARY_DIR}/cli/fxremote-update.exe.bin.c"
    )
    target_link_libraries(p7os PRIVATE ${CLI_LIBRARIES})
    target_include_directories(p7os PRIVATE ${CLI_INCLUDE_DIRS})

    if(ENABLE_SDL)
        add_executable(p7screen
            cli/p7screen.c
            cli/p7screen_args.c
            cli/common.c
            cli/options.c
        )
        target_link_libraries(p7screen PRIVATE
            ${CLI_LIBRARIES}
            ${SDL2_LIBRARIES}
        )
        target_include_directories(p7screen PRIVATE
            ${CLI_INCLUDE_DIRS}
            ${SDL2_INCLUDE_DIRS}
        )
    endif()

    add_executable(xfer9860
        cli/xfer9860.c
        cli/xfer9860_args.c
        cli/common.c
        cli/options.c
    )
    target_link_libraries(xfer9860 PRIVATE ${CLI_LIBRARIES})
    target_include_directories(xfer9860 PRIVATE ${CLI_INCLUDE_DIRS})
endif()

configure_file(misc/cahute.pc.in misc/cahute.pc ESCAPE_QUOTES @ONLY)

# ---
# Packaging options.
# ---

install(TARGETS ${PROJECT_NAME} ARCHIVE)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute.pc
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

if(ENABLE_CLI)
    install(TARGETS p7 RUNTIME)
    install(TARGETS p7os RUNTIME)

    if(ENABLE_SDL)
        install(TARGETS p7screen RUNTIME)
    endif()

    install(TARGETS xfer9860 RUNTIME)

    if(ENABLE_INCOMPLETE_CAS)
        install(TARGETS CaS RUNTIME)
    endif()
endif()

if(ENABLE_UDEV)
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/60-casio-calculators.rules
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/udev/rules.d"
    )
endif()
