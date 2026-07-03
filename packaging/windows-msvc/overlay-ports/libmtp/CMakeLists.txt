cmake_minimum_required(VERSION 3.15)
project(libmtp C)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)

# Generate libmtp.h from the upstream template (fills in version strings etc.)
set(LIBMTP_VERSION_STRING "1.1.22")
configure_file(src/libmtp.h.in "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" @ONLY)

add_library(libmtp STATIC src/msvc_stub.c)

target_include_directories(libmtp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(libmtp PRIVATE ws2_32)

install(TARGETS libmtp ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" DESTINATION include)
