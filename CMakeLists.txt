cmake_minimum_required(VERSION 3.20)
project(SplatoonPort VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recompiler/CMakeLists.txt")
    message(FATAL_ERROR
        "RebrewU submodule is missing. Run: git submodule update --init --recursive")
endif()

include(ExternalProject)
include(CTest)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set(REBREWU_EXTERNAL_ARGS "")
if(REBREWU_EXTERNAL_TOOLCHAIN_FILE)
    # Normalize Windows backslashes before ExternalProject writes its CMake scripts.
    string(REPLACE "\\" "/" REBREWU_TOOLCHAIN_PATH "${REBREWU_EXTERNAL_TOOLCHAIN_FILE}")
    list(APPEND REBREWU_EXTERNAL_ARGS
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${REBREWU_TOOLCHAIN_PATH}"
        "-DVCPKG_TARGET_TRIPLET:STRING=x64-windows-static")
endif()
add_subdirectory(launcher)

# Build upstream in its own CMake project: it uses CMAKE_SOURCE_DIR internally.
# Invoking add_subdirectory(recompiler) would silently break its include paths.
set(REBREWU_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/rebrewu-build")
if(MSVC)
    set(REBREWU_INCLUDE_FIX "/FIalgorithm")
else()
    set(REBREWU_INCLUDE_FIX "-include algorithm")
endif()

ExternalProject_Add(rebrewu_external
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/recompiler"
    BINARY_DIR "${REBREWU_BUILD_DIR}"
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS:STRING=${REBREWU_INCLUDE_FIX}"
        "-DREBREWU_BUILD_TESTS:BOOL=OFF"
        "-DREBREWU_INSTALL:BOOL=OFF"
        "-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded$<$<CONFIG:Debug>:Debug>"
        ${REBREWU_EXTERNAL_ARGS}
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --config $<CONFIG> --target rebrewu --parallel 4
    BUILD_BYPRODUCTS "${REBREWU_BUILD_DIR}/bin/rebrewu${CMAKE_EXECUTABLE_SUFFIX}"
    LOG_CONFIGURE ON
    LOG_BUILD ON
)

# Keep 'cmake --build ... --target SplatoonSetup' a complete two-tool build.
add_dependencies(SplatoonSetup rebrewu_external)

# Only copies public runtime and tools. No game dump or recompiled source.
add_custom_target(package_release
    COMMAND "${CMAKE_COMMAND}"
        "-DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DDIST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/dist"
        "-DLAUNCHER_BINARY=$<TARGET_FILE:SplatoonSetup>"
        "-DREBREWU_BINARY=${REBREWU_BUILD_DIR}/bin/rebrewu${CMAKE_EXECUTABLE_SUFFIX}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageRelease.cmake"
    DEPENDS SplatoonSetup rebrewu_external
    VERBATIM
)
