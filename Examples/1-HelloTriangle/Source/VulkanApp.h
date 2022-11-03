#ifndef VULKANLEARNING_VULKANAPP
#define VULKANLEARNING_VULKANAPP

#include "Log.h"
#include "Window.h"

#include <vulkan/vulkan.h>

class VulkanApp
{
public:
    VulkanApp(int WindowWidth, int WindowHeight);

    void Run();

private:
    void InitVulkan();
    void AppLoop();
    void CleanUp();

private:
    // Vulkan-specific methods
    //=========================================================================================================
    // VK_INSTANCE_RELATED
    void CreateVkInstance(); // Set up VulkanAPI
    void DestroyVkInstance();

    std::vector<char const *> GetRequiredInstanceExtensions() const;
    std::vector<char const *> GetRequiredInstanceValidationLayers() const;

    bool CheckExtensionsAvailable(std::vector<char const *> const &Required) const;
    bool CheckValidationLayersAvailable(std::vector<char const *> const &Required) const;

    std::vector<VkExtensionProperties> GetSupportedExtensionsInfo() const;
    std::vector<VkLayerProperties>     GetSupportedLayersInfo() const;

    void LogSupportedExtensions() const;
    void LogSupportedValidationLayers() const;
    // !VK_INSTANCE_RELATED
    //=========================================================================================================
    // VK_PHYSICAL_DEVICE
    void SelectPhysicalDevice();

    std::vector<VkPhysicalDevice> GetPhysicalDevices() const;
    VkPhysicalDeviceProperties    GetPhysicalDeviceProperties(VkPhysicalDevice PhysicalDevice) const;
    VkPhysicalDeviceFeatures      GetPhysicalDeviceFeatures(VkPhysicalDevice PhysicalDevice) const;
    VkPhysicalDevice              GetMostSuitableDevice(std::vector<VkPhysicalDevice> const &PhysicalDevices) const;
    uint32_t                      GetDeviceSuitability(VkPhysicalDevice PhysicalDevice) const;

    void LogPhysicalDevices() const;
    void LogPhysicalDevice(VkPhysicalDevice PhysicalDevice) const;
    // !VK_PHYSICAL_DEVICE
    //=========================================================================================================
    // VK_QUEUE_FAMILY
    std::vector<VkQueueFamilyProperties> GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice PhysicalDevice) const;

    uint32_t GetMostSuitableQueueFamilyIndex(std::vector<VkQueueFamilyProperties> const &QueueFamiliesProperties) const;
    uint32_t GetQueueFamilySuitability(VkQueueFamilyProperties const &QueueFamilyProperties) const;

    void LogQueueFamiliesProperties(VkPhysicalDevice PhysicalDevice) const;
    void LogQueueFamilyProperties(uint32_t FamilyIndex, VkQueueFamilyProperties QueueFamilyProperties) const;
    // !VK_QUEUE_FAMILY
    //=========================================================================================================
    // VK_DEVICE
    void CreateDevice();
    void DestroyDevice();

    std::vector<char const *> GetRequiredDeviceExtensions() const;
    std::vector<char const *> GetRequiredDeviceValidationLayers(
    ) const; // Ignored by newer Vulkan versions and uses Layers from VkInstance, left for compatibility

    void RetrieveQueueFromCreatedDevice();
    // !VK_DEVICE
    //=========================================================================================================
    // VK_DEBUG_RELATED
    void CreateDebugCallback();
    void DestroyDebugCallback();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      MessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void                                       *pUserData
    );
    // !VK_DEBUG_RELATED

private:
    Window m_Window;

    VkInstance       m_VkInstance{};
    VkPhysicalDevice m_VkPhysicalDevice{};
    uint32_t         m_QueueFamilyIndex = 0;
    VkDevice         m_VkDevice{};
    VkQueue          m_VkQueue;

    VkDebugUtilsMessengerEXT m_VkDebugMessenger{};
};

#endif // !VULKANLEARNING_VULKANAPP
