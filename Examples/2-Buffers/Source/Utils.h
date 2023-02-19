#ifndef VULKANLEARNING_UTILS
#define VULKANLEARNING_UTILS

#include <vulkan/vulkan.h>

namespace Utils
{

    VKAPI_ATTR VkResult VKAPI_CALL CreateDebugUtilsMessengerEXT(
        VkInstance                                Instance,
        VkDebugUtilsMessengerCreateInfoEXT const *pCreateInfo,
        VkAllocationCallbacks const              *pAllocator,
        VkDebugUtilsMessengerEXT                 *pDebugMessenger
    );

    VKAPI_ATTR void VKAPI_CALL DestroyDebugUtilsMessengerEXT(
        VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, VkAllocationCallbacks const *pAllocator
    );

} // namespace Utils

#endif // !VULKANLEARNING_UTILS
