cmake_minimum_required(VERSION 3.20)
project(PortableVolkProbe LANGUAGES C CXX)

# A Vulkan SDK or installed Vulkan library must not be necessary.
include("${CMAKE_CURRENT_LIST_DIR}/../../../port/cmake/PortableVulkan.cmake")

get_target_property(_volk_links volk INTERFACE_LINK_LIBRARIES)
if(NOT "Vulkan::Headers" IN_LIST _volk_links)
    message(FATAL_ERROR "volk does not transitively expose Vulkan::Headers")
endif()

add_executable(portable_volk_probe main.c)
target_link_libraries(portable_volk_probe PRIVATE volk::volk)

enable_testing()
add_test(NAME portable_volk_probe COMMAND portable_volk_probe)
