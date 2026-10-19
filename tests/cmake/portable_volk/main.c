#include <volk.h>
#include <vulkan/vulkan.h>

int main(void) {
    return (VK_HEADER_VERSION >= 296 && sizeof(VkInstance) > 0) ? 0 : 1;
}
