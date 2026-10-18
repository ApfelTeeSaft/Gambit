include_guard(GLOBAL)
include(FetchContent)

if(NOT TARGET ZLIB::ZLIB)
    find_package(ZLIB QUIET)
endif()

if(NOT TARGET ZLIB::ZLIB)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(port_zlib
        URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(port_zlib)

    if(TARGET zlibstatic)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    elseif(TARGET zlib)
        add_library(ZLIB::ZLIB ALIAS zlib)
    endif()
endif()

if(NOT TARGET ZLIB::ZLIB)
    message(FATAL_ERROR "Portable zlib setup failed to provide ZLIB::ZLIB")
endif()
