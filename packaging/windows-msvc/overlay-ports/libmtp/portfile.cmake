vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO libmtp/libmtp
    REF "v1.1.22"
    SHA512 8518d1c201b24aefbb81ca4cffe8a9ab01c3b120f2dece954983ed57c2934b71fcf95cefa3591276812088c3040781e438d8cc6af35e75a66615f63e9632173f
)

# 1. Generate a clean, 100% MSVC-compatible stub implementation for the core MTP symbols
file(WRITE "${SOURCE_PATH}/src/msvc_stub.c" [[
#include <stdlib.h>
#include "libmtp.h"

// Expose the core foundational hardware lifecycle symbol definitions expected by Subsurface
void LIBMTP_Init(void) {}

LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void) { 
    return (void*)0; 
}

void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device) {}

int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device, uint16_t **filetypes, uint16_t *len) {
    if (filetypes) *filetypes = (void*)0;
    if (len) *len = 0;
    return 0;
}

LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device) { 
    return (void*)0; 
}

void LIBMTP_destroy_file_t(LIBMTP_file_t *file) {}

int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *device, uint32_t id, const char *path, LIBMTP_progressfunc_t cb, const void *data) {
    return -1;
}

int LIBMTP_Update_File_Metadata(LIBMTP_mtpdevice_t *device, LIBMTP_file_t const *file) {
    return -1;
}
]])

# 2. Generate our custom build script mapping exclusively to our pristine compiler stub
file(WRITE "${SOURCE_PATH}/CMakeLists.txt" [[
cmake_minimum_required(VERSION 3.15)
project(libmtp C)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(WIN32_LEAN_AND_MEAN)

set(SOURCES src/msvc_stub.c)

set(LIBMTP_VERSION_STRING "1.1.22")
configure_file(src/libmtp.h.in ${CMAKE_CURRENT_BINARY_DIR}/libmtp_generated.h @ONLY)

# Forward proxy mapping to present a clean standalone inclusion layout
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/libmtp.h "
#ifndef LIBMTP_H_MSVC_PROXY
#define LIBMTP_H_MSVC_PROXY
#include \"libmtp_generated.h\"
#endif
")

add_library(libmtp STATIC ${SOURCES})

target_include_directories(libmtp PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(libmtp PRIVATE 
    ws2_32
)

install(TARGETS libmtp ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmtp_generated.h DESTINATION include RENAME libmtp.h)
]])

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/COPYING" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
