cmake_minimum_required(VERSION 3.20)
project(PortableZlibProbe LANGUAGES C)

# The CI invocation disables package discovery to exercise the source fallback.
include("${CMAKE_CURRENT_LIST_DIR}/../../../port/cmake/PortableZlib.cmake")

if(CMAKE_DISABLE_FIND_PACKAGE_ZLIB AND NOT TARGET zlibstatic)
    message(FATAL_ERROR "Expected the fetched static zlib target")
endif()

add_executable(portable_zlib_probe main.c)
target_link_libraries(portable_zlib_probe PRIVATE ZLIB::ZLIB)

enable_testing()
add_test(NAME portable_zlib_probe COMMAND portable_zlib_probe)
