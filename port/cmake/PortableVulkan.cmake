include_guard(GLOBAL)
include(FetchContent)

# The native Vulkan runtime must build without requiring a system-installed SDK.
set(VULKAN_HEADERS_ENABLE_MODULE OFF CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(port_vulkan_headers
    URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.3.296.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(port_vulkan_headers)

if(NOT TARGET Vulkan::Headers)
    message(FATAL_ERROR "The fetched Vulkan-Headers source did not provide Vulkan::Headers")
endif()

# volk does not automatically consume the already-created Vulkan::Headers target.
# Disable its SDK discovery and explicitly propagate the bundled header interface.
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(port_volk
    URL https://github.com/zeux/volk/archive/5283d1ba45f1886be8f4e133f211929c8a7c099b.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(port_volk)

if(NOT TARGET volk::volk OR NOT TARGET volk)
    message(FATAL_ERROR "The fetched volk source did not provide volk::volk")
endif()
target_link_libraries(volk PUBLIC Vulkan::Headers)

