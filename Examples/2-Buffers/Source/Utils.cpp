#include "Utils.h"

namespace Utils
{
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance const                                Instance,
        VkDebugUtilsMessengerCreateInfoEXT const *const pCreateInfo,
        VkAllocationCallbacks const *const              pAllocator,
        VkDebugUtilsMessengerEXT *const                 pDebugMessenger
    )
    {
        PFN_vkCreateDebugUtilsMessengerEXT const CreateFunc =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
        if (CreateFunc)
        {
            return CreateFunc(Instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, VkAllocationCallbacks const *pAllocator
    )
    {
        PFN_vkDestroyDebugUtilsMessengerEXT DestroyFunc =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
        if (DestroyFunc)
        {
            DestroyFunc(Instance, DebugMessenger, pAllocator);
        }
    }
} // namespace Utils
