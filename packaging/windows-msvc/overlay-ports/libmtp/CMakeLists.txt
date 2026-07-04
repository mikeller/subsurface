cmake_minimum_required(VERSION 3.15)
project(libmtp CXX)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)

# Generate libmtp.h from the upstream template (fills in version strings etc.)
set(LIBMTP_VERSION_STRING "1.1.22")
configure_file(src/libmtp.h.in "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" @ONLY)
if(MSVC)
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" LIBMTP_HEADER)
    string(REPLACE [=[
/*
 * Types that do not exist in Windows
 * sys/types.h, but they exist in mingw32
 * sys/types.h.
 */
typedef char int8_t;
typedef unsigned char uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
]=] "" LIBMTP_HEADER "${LIBMTP_HEADER}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" "${LIBMTP_HEADER}")
endif()

add_library(libmtp STATIC src/msvc_wpd.cpp)

target_include_directories(libmtp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_features(libmtp PRIVATE cxx_std_17)
target_link_libraries(libmtp PRIVATE
    ole32
    portabledeviceguids
    propsys
)

install(TARGETS libmtp ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" DESTINATION include)
