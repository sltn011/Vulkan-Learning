#ifndef VULKANLEARNING_SWAPCHAINSUPPORTDETAILS
#define VULKANLEARNING_SWAPCHAINSUPPORTDETAILS

#include <vector>
#include <vulkan/vulkan.h>

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> SurfaceFormats;
    std::vector<VkPresentModeKHR>   PresentationMode;
};

#endif // !VULKANLEARNING_SWAPCHAINSUPPORTDETAILS
