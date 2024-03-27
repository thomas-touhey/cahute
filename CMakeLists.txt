cmake_minimum_required(VERSION 3.16)

project(
    cahute
    VERSION 0.2
    DESCRIPTION "Communication and file format handling tools for CASIO calculators"
    HOMEPAGE_URL "https://cahuteproject.org/"
    LANGUAGES C
)
set(PROJECT_ISSUES_URL "https://cahuteproject.org/guides/report.html")

add_custom_target(lint COMMAND pre-commit run --all-files)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)

set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(
    CPACK_SOURCE_IGNORE_FILES
    "\\.git" "build/" ".*~$" "\\.envrc" "\\.pre-commit-config\\.yaml"
    "\\.clang-format" "__pycache__" "\\.gitlab-ci.yml" "_CPack"
)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_VERBATIM_VARIABLES true)
include(CPack)

set(LOGLEVEL $ENV{LOGLEVEL})

if(NOT "${LOGLEVEL}" MATCHES "|none|info|warning|error|fatal")
    message(SEND_ERROR "LOGLEVEL should be one of [info, warning, error, fatal, none]")
endif()

if(NOT LOGLEVEL)
    set(LOGLEVEL "fatal")
endif()

string(TOUPPER ${LOGLEVEL} DEFAULT_LOGLEVEL)

execute_process(
  COMMAND printf "0x%02X%02X0000" ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR}
  OUTPUT_VARIABLE PROJECT_VERSION_HEX
)

configure_file(include/cahute/config.h.in include/cahute/config.h ESCAPE_QUOTES)
configure_file(misc/cahute.pc.in misc/cahute.pc ESCAPE_QUOTES @ONLY)

# ---
# Build options.
# ---

pkg_check_modules(libusb REQUIRED libusb-1.0 IMPORTED_TARGET)

set(CMAKE_C_STANDARD 90)
add_compile_options(
    -Wall -Wextra -Wno-unused-macros -Wno-vla
    -Wshadow -Wwrite-strings -Wredundant-decls -Wformat -Wformat-nonliteral
    -Wformat-security -Wimplicit-function-declaration
    -Wdate-time -Wmissing-prototypes -Wreturn-type -Wpointer-arith
    -Wstack-protector -Wno-unused-parameter
)

add_library(${PROJECT_NAME} STATIC
    lib/casiolink.c
    lib/cdefs.c
    lib/detection.c
    lib/link.c
    lib/linkopen.c
    lib/logging.c
    lib/misc.c
    lib/picture.c
    lib/seven.c
    lib/seven_ohp.c
    lib/stream.c
)

add_executable(p7
    cli/p7.c
    cli/p7_args.c
    cli/common.c
    cli/options.c
)

add_executable(p7os
    cli/p7os.c
    cli/p7os_args.c
    cli/common.c
    cli/options.c
    cli/fxremote-update.exe.bin.c
)

add_executable(p7screen
    cli/p7screen.c
    cli/p7screen_args.c
    cli/common.c
    cli/options.c
)

add_executable(xfer9860
    cli/xfer9860.c
    cli/xfer9860_args.c
    cli/common.c
    cli/options.c
)

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${libusb_INCLUDE_DIRS}"
)
target_include_directories(p7screen PRIVATE ${SDL2_INCLUDE_DIR})

target_link_libraries(p7 PRIVATE ${PROJECT_NAME} PkgConfig::libusb)
target_link_libraries(p7os PRIVATE ${PROJECT_NAME} PkgConfig::libusb)
target_link_libraries(p7screen PRIVATE ${PROJECT_NAME} PkgConfig::libusb ${SDL2_LIBRARIES})
target_link_libraries(xfer9860 PRIVATE ${PROJECT_NAME} PkgConfig::libusb)

# ---
# Packaging options.
# ---

install(TARGETS ${PROJECT_NAME} ARCHIVE)
install(TARGETS p7 RUNTIME)
install(TARGETS p7os RUNTIME)
install(TARGETS p7screen RUNTIME)
install(TARGETS xfer9860 RUNTIME)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/misc/cahute.pc
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/60-casio-calculators.rules
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/udev/rules.d"
)
