#include "VulkanApp.h"

#include "Utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// #include <vulkan/vulkan.h> - redundant, included by GLFW

#include <cstdint>
#include <string>
#include <vector>

#ifdef VKL_DEBUG
constexpr bool g_bValidationLayersEnabled = true;
#else
constexpr bool g_bValidationLayersEnabled = false;
#endif

VulkanApp::VulkanApp(int const WindowWidth, int const WindowHeight)
    : m_Window(WindowWidth, WindowHeight, "1-HelloTriangle")
{
    VKL_INFO("VulkanApp created");
}

void VulkanApp::Run()
{
    InitVulkan();
    AppLoop();
    CleanUp();
}

void VulkanApp::InitVulkan()
{
    VKL_INFO("Initializing Vulkan...");

    if (!glfwVulkanSupported())
    {
        VKL_CRITICAL("Vulkan not supported!");
        exit(1);
    }

    LogSupportedExtensions();
    LogSupportedValidationLayers();

    CreateVkInstance();
    CreateDebugCallback();
    SelectPhysicalDevice();
    CreateDevice();
    RetrieveQueueFromCreatedDevice();

    VKL_INFO("Vulkan initialized");
}

void VulkanApp::AppLoop()
{
    while (!glfwWindowShouldClose(m_Window.Get()))
    {
        glfwPollEvents();
    }
}

void VulkanApp::CleanUp()
{
    VKL_INFO("VulkanApp is stopping...");

    DestroyDevice();

    DestroyDebugCallback();

    DestroyVkInstance();

    VKL_INFO("VkInstance destroyed");
}

void VulkanApp::CreateVkInstance()
{
    VKL_TRACE("Creating VkInstance...");

    VkApplicationInfo ApplicationInfo{};
    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.apiVersion         = VK_API_VERSION_1_3;
    ApplicationInfo.pApplicationName   = m_Window.GetTitle();
    ApplicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    ApplicationInfo.pEngineName        = "";
    ApplicationInfo.engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0);

    std::vector<char const *> const Extensions       = GetRequiredInstanceExtensions();
    std::vector<char const *> const ValidationLayers = GetRequiredInstanceValidationLayers();

    // Specify global(for whole program) extensions and validation layers
    VkInstanceCreateInfo InstanceCreateInfo{};
    InstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo        = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(Extensions.size());
    InstanceCreateInfo.ppEnabledExtensionNames = Extensions.data();
    InstanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(ValidationLayers.size());
    InstanceCreateInfo.ppEnabledLayerNames     = ValidationLayers.data();

    if (vkCreateInstance(&InstanceCreateInfo, nullptr, &m_VkInstance) != VkResult::VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkInstance!");
        exit(1);
    }

    VKL_TRACE("Created VkInstance successfully");
}

void VulkanApp::DestroyVkInstance()
{
    vkDestroyInstance(m_VkInstance, nullptr);
}

std::vector<char const *> VulkanApp::GetRequiredInstanceExtensions() const
{
    uint32_t     NumRequiredExtensions = 0;
    char const **RequiredExtensions    = glfwGetRequiredInstanceExtensions(&NumRequiredExtensions);

    // clang-format off
    const std::vector<char const *> AdditionalExtensions
    {
        "VK_EXT_debug_utils"
    };
    uint32_t NumAdditionalExtensions = static_cast<uint32_t>(AdditionalExtensions.size());
    // clang-format on

    std::vector<char const *> Extensions(NumRequiredExtensions + NumAdditionalExtensions);
    VKL_TRACE("Required instance extensions: ");
    for (uint32_t i = 0; i < NumRequiredExtensions; ++i)
    {
        VKL_TRACE("{}: {}", i + 1, RequiredExtensions[i]);
        Extensions[i] = RequiredExtensions[i];
    }
    VKL_TRACE("Additional instance extensions: ");
    for (uint32_t i = 0; i < NumAdditionalExtensions; ++i)
    {
        VKL_TRACE("{}: {}", i + 1, AdditionalExtensions[i]);
        Extensions[NumRequiredExtensions + i] = AdditionalExtensions[i];
    }

    if (!CheckExtensionsAvailable(Extensions))
    {
        VKL_CRITICAL("Not all required extensions are available!");
        exit(1);
    }

    return Extensions;
}

std::vector<char const *> VulkanApp::GetRequiredInstanceValidationLayers() const
{
    if constexpr (g_bValidationLayersEnabled)
    {
        // clang-format off
        const std::vector<char const *> ValidationLayers
        {
            "VK_LAYER_KHRONOS_validation"
        };
        // clang-format on

        if (!CheckValidationLayersAvailable(ValidationLayers))
        {
            VKL_CRITICAL("Not all required validation layers are available!");
            exit(1);
        }

        return ValidationLayers;
    }
    else
    {
        return {};
    }
}

bool VulkanApp::CheckExtensionsAvailable(std::vector<char const *> const &Required) const
{
    std::vector<VkExtensionProperties> const Supported = GetSupportedExtensionsInfo();

    for (char const *RequiredExtName : Required)
    {
        bool bIsSupported = false;
        for (VkExtensionProperties const &SupportedExt : Supported)
        {
            if (!std::strcmp(RequiredExtName, SupportedExt.extensionName))
            {
                bIsSupported = true;
                break;
            }
        }
        if (!bIsSupported)
        {
            VKL_ERROR("Extension \"{}\" not supported!", RequiredExtName);
            return false;
        }
    }
    return true;
}

bool VulkanApp::CheckValidationLayersAvailable(std::vector<char const *> const &Required) const
{
    std::vector<VkLayerProperties> const Supported = GetSupportedLayersInfo();

    for (char const *RequiredLayerName : Required)
    {
        bool bIsSupported = false;
        for (VkLayerProperties const &SupportedLayer : Supported)
        {
            if (!std::strcmp(RequiredLayerName, SupportedLayer.layerName))
            {
                bIsSupported = true;
                break;
            }
        }
        if (!bIsSupported)
        {
            VKL_ERROR("Validation layer \"{}\" not supported!", RequiredLayerName);
            return false;
        }
    }
    return true;
}

std::vector<VkExtensionProperties> VulkanApp::GetSupportedExtensionsInfo() const
{
    uint32_t NumExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &NumExtensions, nullptr);

    std::vector<VkExtensionProperties> Extensions(NumExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &NumExtensions, Extensions.data());
    return Extensions;
}

std::vector<VkLayerProperties> VulkanApp::GetSupportedLayersInfo() const
{
    uint32_t NumLayers = 0;
    vkEnumerateInstanceLayerProperties(&NumLayers, nullptr);

    std::vector<VkLayerProperties> Layers(NumLayers);
    vkEnumerateInstanceLayerProperties(&NumLayers, Layers.data());
    return Layers;
}

void VulkanApp::LogSupportedExtensions() const
{
    std::vector<VkExtensionProperties> const Extensions = GetSupportedExtensionsInfo();

    VKL_TRACE("Vulkan supported extensions:");
    uint32_t Counter = 1;
    for (VkExtensionProperties const &Extension : Extensions)
    {
        VKL_TRACE("{}: {}, v.{}", Counter++, Extension.extensionName, Extension.specVersion);
    }
}

void VulkanApp::LogSupportedValidationLayers() const
{
    std::vector<VkLayerProperties> const Layers = GetSupportedLayersInfo();

    VKL_TRACE("Vulkan validation layers:");
    uint32_t Counter = 1;
    for (VkLayerProperties const &Layer : Layers)
    {
        VKL_TRACE("{}: {}, v.{}", Counter++, Layer.layerName, Layer.implementationVersion);
    }
}

void VulkanApp::SelectPhysicalDevice()
{
    std::vector<VkPhysicalDevice> const PhysicalDevices = GetPhysicalDevices();

    m_VkPhysicalDevice = GetMostSuitableDevice(PhysicalDevices);

    VKL_TRACE("Selected VkPhysicalDevice:");
    LogPhysicalDevice(m_VkPhysicalDevice);
}

std::vector<VkPhysicalDevice> VulkanApp::GetPhysicalDevices() const
{
    uint32_t NumPhysicalDevices = 0;
    vkEnumeratePhysicalDevices(m_VkInstance, &NumPhysicalDevices, nullptr);

    if (NumPhysicalDevices == 0)
    {
        VKL_CRITICAL("No VkPhysicalDevice found!");
        exit(1);
    }

    std::vector<VkPhysicalDevice> PhysicalDevices(NumPhysicalDevices);
    vkEnumeratePhysicalDevices(m_VkInstance, &NumPhysicalDevices, PhysicalDevices.data());

    return PhysicalDevices;
}

VkPhysicalDeviceProperties VulkanApp::GetPhysicalDeviceProperties(VkPhysicalDevice PhysicalDevice) const
{
    VkPhysicalDeviceProperties PhysicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
    return PhysicalDeviceProperties;
}

VkPhysicalDeviceFeatures VulkanApp::GetPhysicalDeviceFeatures(VkPhysicalDevice PhysicalDevice) const
{
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures{};
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceFeatures);
    return PhysicalDeviceFeatures;
}

VkPhysicalDevice VulkanApp::GetMostSuitableDevice(std::vector<VkPhysicalDevice> const &PhysicalDevices) const
{
    VkPhysicalDevice MostSuitableDevice      = VK_NULL_HANDLE;
    uint32_t         HighestSuitabilityScore = 0;

    for (VkPhysicalDevice const PhysicalDevice : PhysicalDevices)
    {
        uint32_t SuitabilityScore = GetDeviceSuitability(PhysicalDevice);
        if (SuitabilityScore > HighestSuitabilityScore)
        {
            MostSuitableDevice      = PhysicalDevice;
            HighestSuitabilityScore = SuitabilityScore;
        }
    }

    if (MostSuitableDevice == VK_NULL_HANDLE)
    {
        VKL_CRITICAL("No suitable VkPhysicalDevice found!");
        exit(1);
    }

    return MostSuitableDevice;
}

uint32_t VulkanApp::GetDeviceSuitability(VkPhysicalDevice const PhysicalDevice) const
{
    uint32_t Score = 0;

    VkPhysicalDeviceProperties const PhysicalDeviceProperties = GetPhysicalDeviceProperties(PhysicalDevice);
    VkPhysicalDeviceFeatures const   PhysicalDeviceFeatures   = GetPhysicalDeviceFeatures(PhysicalDevice);

    if (!PhysicalDeviceFeatures.geometryShader)
    {
        return 0; // Discard PhysicalDevice without Geometry Shader support
    }

    switch (PhysicalDeviceProperties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        Score += 1;
        break;

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        Score += 10000;
        break;

    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        Score += 100;
        break;

    default:
        break;
    }

    // Biggest 2D Texture dimensions affect image quality
    Score += PhysicalDeviceProperties.limits.maxImageDimension2D;

    return Score;
}

void VulkanApp::LogPhysicalDevices() const
{
    std::vector<VkPhysicalDevice> PhysicalDevices = GetPhysicalDevices();
    for (VkPhysicalDevice PhysicalDevice : PhysicalDevices)
    {
        LogPhysicalDevice(PhysicalDevice);
    }
}

void VulkanApp::LogPhysicalDevice(VkPhysicalDevice PhysicalDevice) const
{
    VkPhysicalDeviceProperties const PhysicalDeviceProperties = GetPhysicalDeviceProperties(PhysicalDevice);
    VkPhysicalDeviceFeatures const   PhysicalDeviceFeatures   = GetPhysicalDeviceFeatures(PhysicalDevice);

    std::string DeviceType;
    switch (PhysicalDeviceProperties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        DeviceType = "CPU";
        break;

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        DeviceType = "Discrete GPU";
        break;

    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        DeviceType = "Integrated GPU";
        break;

    default:
        DeviceType = "Unknown type";
        break;
    }

    VKL_TRACE(
        "VKPhysicalDevice - Name: {}, Type: {}, API: v{}, VendorID: {}, Driver: v{}",
        PhysicalDeviceProperties.deviceName,
        DeviceType,
        PhysicalDeviceProperties.apiVersion,
        PhysicalDeviceProperties.vendorID,
        PhysicalDeviceProperties.driverVersion
    );
}

std::vector<VkQueueFamilyProperties> VulkanApp::GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice const PhysicalDevice
) const
{
    uint32_t NumQueueFamilyProperties = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &NumQueueFamilyProperties, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilyProperties(NumQueueFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &NumQueueFamilyProperties, QueueFamilyProperties.data());
    return QueueFamilyProperties;
}

uint32_t VulkanApp::GetMostSuitableQueueFamilyIndex(std::vector<VkQueueFamilyProperties> const &QueueFamiliesProperties
) const
{
    uint32_t MostSuitableQueueFamilyIndex; // No default value
    uint32_t HighestSuitabilityScore = 0;

    uint32_t Counter = 0;
    for (VkQueueFamilyProperties const &QueueFamilyProperties : QueueFamiliesProperties)
    {
        uint32_t QueueFamilySuitability = GetQueueFamilySuitability(QueueFamilyProperties);
        if (QueueFamilySuitability > HighestSuitabilityScore)
        {
            MostSuitableQueueFamilyIndex = Counter;
            HighestSuitabilityScore      = QueueFamilySuitability;
        }
    }

    if (HighestSuitabilityScore == 0)
    {
        VKL_CRITICAL("No suitable queue family found!");
        exit(1);
    }

    return MostSuitableQueueFamilyIndex;
}

uint32_t VulkanApp::GetQueueFamilySuitability(VkQueueFamilyProperties const &QueueFamilyProperties) const
{
    uint32_t           Score      = 0;
    VkQueueFlags const QueueFlags = QueueFamilyProperties.queueFlags;

    if (!(QueueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
        return 0; // Required
    }

    if (QueueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        Score += 10;
    }
    if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        Score += 10;
    }
    if (QueueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
    {
        Score += 10;
    }

    Score += QueueFamilyProperties.queueCount;
    return Score;
}

void VulkanApp::LogQueueFamiliesProperties(VkPhysicalDevice const PhysicalDevice) const
{
    std::vector<VkQueueFamilyProperties> QueueFamilesProperties =
        GetPhysicalDeviceQueueFamilyProperties(PhysicalDevice);

    uint32_t FamilyIndex = 0;
    for (VkQueueFamilyProperties const &QueueFamilyProperties : QueueFamilesProperties)
    {
        LogQueueFamilyProperties(FamilyIndex++, QueueFamilyProperties);
    }
}

void VulkanApp::LogQueueFamilyProperties(
    uint32_t const FamilyIndex, VkQueueFamilyProperties const QueueFamilyProperties
) const
{
    VkQueueFlags const QueueFlags = QueueFamilyProperties.queueFlags;
    VKL_TRACE(
        "QueueFamily {}: QueueCount: {}, Bits: [Graphics: {:b}, Compute: {:b}, Transfer: {:b}, SparseBinding: {:b}]",
        FamilyIndex,
        QueueFamilyProperties.queueCount,
        static_cast<bool>(QueueFlags & VK_QUEUE_GRAPHICS_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_COMPUTE_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_TRANSFER_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
    );
}

void VulkanApp::CreateDevice()
{
    std::vector<VkQueueFamilyProperties> QueueFamilyProperties =
        GetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice);

    float QueuePriority = 1.0f;

    VkDeviceQueueCreateInfo DeviceQueueCreateInfo{};
    DeviceQueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    DeviceQueueCreateInfo.queueFamilyIndex = GetMostSuitableQueueFamilyIndex(QueueFamilyProperties);
    DeviceQueueCreateInfo.queueCount       = 1;
    DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;

    // Features supported by VkPhysicalDevice that are requested for use by VkDevice
    VkPhysicalDeviceFeatures DeviceRequestedFeatures{};

    std::vector<char const *> Extensions       = GetRequiredDeviceExtensions();
    std::vector<char const *> ValidationLayers = GetRequiredDeviceValidationLayers();

    VkDeviceCreateInfo DeviceCreateInfo{};
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pQueueCreateInfos       = &DeviceQueueCreateInfo;
    DeviceCreateInfo.queueCreateInfoCount    = 1;
    DeviceCreateInfo.pEnabledFeatures        = &DeviceRequestedFeatures;
    DeviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(Extensions.size());
    DeviceCreateInfo.ppEnabledExtensionNames = Extensions.data();
    DeviceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(ValidationLayers.size());
    DeviceCreateInfo.ppEnabledLayerNames     = ValidationLayers.data();

    if (vkCreateDevice(m_VkPhysicalDevice, &DeviceCreateInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VKDevice!");
        exit(1);
    }
}

void VulkanApp::DestroyDevice()
{
    vkDestroyDevice(m_VkDevice, nullptr);
}

std::vector<char const *> VulkanApp::GetRequiredDeviceExtensions() const
{
    // clang-format off
    std::vector<char const *> DeviceExtensions
    {

    };
    // clang-format on
    return DeviceExtensions;
}

std::vector<char const *> VulkanApp::GetRequiredDeviceValidationLayers() const
{
    return GetRequiredInstanceValidationLayers(); // same as Instance Layers
}

void VulkanApp::RetrieveQueueFromCreatedDevice()
{
    uint32_t QueueIndex = 0;
    vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndex, QueueIndex, &m_VkQueue);
}

void VulkanApp::CreateDebugCallback()
{
    if constexpr (!g_bValidationLayersEnabled)
    {
        return;
    }
    else
    {
        // clang-format off
        VkDebugUtilsMessengerCreateInfoEXT MessengerInfo{};
        MessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        MessengerInfo.messageSeverity =
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        MessengerInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        MessengerInfo.pfnUserCallback = VulkanApp::DebugCallback;
        MessengerInfo.pUserData       = nullptr;
        // clang-format on

        if (Utils::CreateDebugUtilsMessengerEXT(m_VkInstance, &MessengerInfo, nullptr, &m_VkDebugMessenger) !=
            VK_SUCCESS)
        {
            VKL_CRITICAL("Failed to create VkDebugUtilsMessengerEXT!");
            exit(1);
        }

        VKL_TRACE("Debug callback set up successfully");
    }
}

void VulkanApp::DestroyDebugCallback()
{
    Utils::DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);
}

VkBool32 VulkanApp::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT const      MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT const             MessageType,
    const VkDebugUtilsMessengerCallbackDataEXT *const pCallbackData,
    void *const                                       pUserData
)
{
    switch (MessageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        VKL_TRACE(pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        VKL_INFO(pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        VKL_WARN(pCallbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        VKL_ERROR(pCallbackData->pMessage);
        break;

    default:
        break;
    }
    return VK_FALSE;
}
