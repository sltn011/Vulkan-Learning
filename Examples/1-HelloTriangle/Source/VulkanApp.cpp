#include "VulkanApp.h"

#include "Utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// #include <vulkan/vulkan.h> - redundant, included by GLFW

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

    CreateSurface();
    SelectPhysicalDevice();
    CreateDevice();
    RetrieveQueuesFromDevice();

    CreateSwapchain();
    RetrieveSwapchainImages();
    CreateSwapchainImagesViews();

    CreateRenderPass();
    CreatePipelineLayout();
    CreatePipeline();

    CreateFramebuffer();

    CreateCommandPool();
    AllocateCommandBuffers();

    CreateSyncObjects();

    VKL_INFO("Vulkan initialized");
}

void VulkanApp::AppLoop()
{
    VKL_INFO("VulkanApp running");
    while (!glfwWindowShouldClose(m_Window.Get()))
    {
        DrawFrame();
        glfwPollEvents();
    }

    vkDeviceWaitIdle(m_VkDevice);
}

void VulkanApp::CleanUp()
{
    VKL_INFO("VulkanApp is stopping...");

    DestroySyncObjects();

    DestroyCommandPool();

    DestroyFramebuffer();

    DestroyPipeline();
    DestroyPipelineLayout();
    DestroyRenderPass();

    DestroySwapchainImagesViews();
    DestroySwapchain();

    DestroyDevice();
    DestroySurface();

    DestroyDebugCallback();

    DestroyVkInstance();

    VKL_INFO("VulkanApp resources cleaned up");
}

void VulkanApp::DrawFrame()
{
    /*
    1) Wait for the previous frame to finish
    2) Acquire an image from the swap chain
    3) Record a command buffer which draws the scene onto that image
    4) Submit the recorded command buffer
    5) Present the swap chain image
    */

    // 1
    vkWaitForFences(m_VkDevice, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_VkDevice, 1, &m_InFlightFences[m_CurrentFrame]);

    // 2
    uint32_t SwapchainImageIndex = 0;
    vkAcquireNextImageKHR(
        m_VkDevice,
        m_VkSwapchain,
        UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame],
        VK_NULL_HANDLE,
        &SwapchainImageIndex
    );

    // 3
    vkResetCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], 0);
    RecordCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], SwapchainImageIndex);

    // 4
    SubmitCommandBuffer(m_VkCommandBuffers[m_CurrentFrame]);

    // 5
    PresentResult(SwapchainImageIndex);

    m_CurrentFrame = (m_CurrentFrame + 1) % s_FramesInFlight;
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

    if (vkCreateInstance(&InstanceCreateInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkInstance!");
        exit(1);
    }
    VKL_TRACE("Created VkInstance successfully");
}

void VulkanApp::DestroyVkInstance()
{
    vkDestroyInstance(m_VkInstance, nullptr);
    VKL_TRACE("VkInstance destroyed");
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

    m_VkPhysicalDevice   = GetMostSuitablePhysicalDevice(PhysicalDevices);
    m_QueueFamilyIndices = GetPhysicalDeviceMostSuitableQueueFamilyIndices(m_VkPhysicalDevice);

    VKL_TRACE("Selected VkPhysicalDevice:");
    LogPhysicalDevice(m_VkPhysicalDevice);
    VKL_TRACE("Selected VkPhysicalDevice QueueFamilyProperties:");
    LogPhysicalDeviceQueueFamiliesProperties(m_VkPhysicalDevice);
    VKL_TRACE("Selected VkPhysicalDevice supported extensions:");
    LogPhysicalDeviceSupportedExtensions(m_VkPhysicalDevice);
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

std::vector<VkExtensionProperties> VulkanApp::GetPhysicalDeviceSupportedExtensions(
    VkPhysicalDevice PhysicalDevice
) const
{
    uint32_t NumSupportedExtensions;
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &NumSupportedExtensions, nullptr);

    std::vector<VkExtensionProperties> SupportedExtensions(NumSupportedExtensions);
    vkEnumerateDeviceExtensionProperties(
        PhysicalDevice, nullptr, &NumSupportedExtensions, SupportedExtensions.data()
    );

    return SupportedExtensions;
}

VkPhysicalDevice VulkanApp::GetMostSuitablePhysicalDevice(std::vector<VkPhysicalDevice> const &PhysicalDevices
) const
{
    VkPhysicalDevice MostSuitableDevice      = VK_NULL_HANDLE;
    uint32_t         HighestSuitabilityScore = 0;

    for (VkPhysicalDevice const PhysicalDevice : PhysicalDevices)
    {
        bool bPhysicalDeviceSuitable = IsPhysicalDeviceSuitable(PhysicalDevice);
        if (!bPhysicalDeviceSuitable)
        {
            continue;
        }

        uint32_t SuitabilityScore = GetPhysicalDeviceSuitability(PhysicalDevice);
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

bool VulkanApp::IsPhysicalDeviceSuitable(VkPhysicalDevice PhysicalDevice) const
{
    QueueFamilyIndices FamiliesIndices = GetPhysicalDeviceMostSuitableQueueFamilyIndices(PhysicalDevice);
    bool               bAllExtensionsSupported = IsPhysicalDeviceExtensionSupportComplete(PhysicalDevice);

    bool bSwapchainSuitable = false;
    if (bAllExtensionsSupported)
    {
        SwapchainSupportDetails SwapchainSupport = GetSwapchainSupportDetails(PhysicalDevice, m_VkSurface);
        bSwapchainSuitable =
            !SwapchainSupport.PresentationMode.empty() && !SwapchainSupport.SurfaceFormats.empty();
    }

    return FamiliesIndices.IsComplete() && bAllExtensionsSupported && bSwapchainSuitable;
}

bool VulkanApp::IsPhysicalDeviceExtensionSupportComplete(VkPhysicalDevice PhysicalDevice) const
{
    std::vector<char const *>          RequiredExtensions = GetRequiredDeviceExtensions();
    std::vector<VkExtensionProperties> SupportedExtensions =
        GetPhysicalDeviceSupportedExtensions(PhysicalDevice);

    for (char const *RequiredExtension : RequiredExtensions)
    {
        bool IsSupported = false;
        for (VkExtensionProperties const &SupportedExtension : SupportedExtensions)
        {
            if (std::strcmp(RequiredExtension, SupportedExtension.extensionName) == 0)
            {
                IsSupported = true;
                break;
            }
        }
        if (!IsSupported)
        {
            return false;
        }
    }

    return true;
}

uint32_t VulkanApp::GetPhysicalDeviceSuitability(VkPhysicalDevice const PhysicalDevice) const
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

void VulkanApp::LogPhysicalDeviceSupportedExtensions(VkPhysicalDevice PhysicalDevice) const
{
    std::vector<VkExtensionProperties> SupportedExtensions =
        GetPhysicalDeviceSupportedExtensions(PhysicalDevice);

    VKL_TRACE("VkPhysicalDevice supported extensions:");
    size_t Counter = 0;
    for (VkExtensionProperties const &Extension : SupportedExtensions)
    {
        VKL_TRACE("{}: {} v.{}", Counter++, Extension.extensionName, Extension.specVersion);
    }
}

std::vector<VkQueueFamilyProperties> VulkanApp::GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice const PhysicalDevice
) const
{
    uint32_t NumQueueFamilyProperties = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &NumQueueFamilyProperties, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilyProperties(NumQueueFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(
        PhysicalDevice, &NumQueueFamilyProperties, QueueFamilyProperties.data()
    );
    return QueueFamilyProperties;
}

QueueFamilyIndices VulkanApp::GetPhysicalDeviceMostSuitableQueueFamilyIndices(VkPhysicalDevice PhysicalDevice
) const
{
    std::vector<VkQueueFamilyProperties> QueueFamiliesProperties =
        GetPhysicalDeviceQueueFamilyProperties(PhysicalDevice);

    QueueFamilyIndices FamilyIndices{};
    FamilyIndices.GraphicsFamily = GetPhysicalDeviceMostSuitableQueueFamily(
        PhysicalDevice, QueueFamiliesProperties, &VulkanApp::GetPhysicalDeviceGraphicsQueueFamilySuitability
    );
    FamilyIndices.PresentationFamily = GetPhysicalDeviceMostSuitableQueueFamily(
        PhysicalDevice,
        QueueFamiliesProperties,
        &VulkanApp::GetPhysicalDevicePresentationQueueFamilySuitability
    );
    return FamilyIndices;
}

std::optional<uint32_t> VulkanApp::GetPhysicalDeviceMostSuitableQueueFamily(
    VkPhysicalDevice                            PhysicalDevice,
    std::vector<VkQueueFamilyProperties> const &QueueFamiliesProperties,
    uint32_t (VulkanApp::*SuitabilityCalculatorFunction
    )(VkPhysicalDevice const, VkQueueFamilyProperties const &, uint32_t) const
) const
{
    std::optional<uint32_t> QueueFamilyIndex;
    uint32_t                HighestSuitabilityScore = 0;

    uint32_t FamilyIndex = 0;
    for (VkQueueFamilyProperties const &QueueFamilyProperties : QueueFamiliesProperties)
    {
        uint32_t QueueFamilySuitability =
            (this->*SuitabilityCalculatorFunction)(PhysicalDevice, QueueFamilyProperties, FamilyIndex);
        if (QueueFamilySuitability > HighestSuitabilityScore)
        {
            QueueFamilyIndex        = FamilyIndex;
            HighestSuitabilityScore = QueueFamilySuitability;
        }
        FamilyIndex++;
    }

    return QueueFamilyIndex;
}

uint32_t VulkanApp::GetPhysicalDeviceGraphicsQueueFamilySuitability(
    VkPhysicalDevice               PhysicalDevice,
    VkQueueFamilyProperties const &QueueFamilyProperties,
    uint32_t                       QueueFamilyIndex
) const
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

    VkBool32 bPresentationSupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        PhysicalDevice, QueueFamilyIndex, m_VkSurface, &bPresentationSupported
    );
    if (bPresentationSupported)
    {
        Score += 100;
    }

    Score += QueueFamilyProperties.queueCount;
    return Score;
}

uint32_t VulkanApp::GetPhysicalDevicePresentationQueueFamilySuitability(
    VkPhysicalDevice               PhysicalDevice,
    VkQueueFamilyProperties const &QueueFamilyProperties,
    uint32_t                       QueueFamilyIndex
) const
{
    uint32_t           Score      = 0;
    VkQueueFlags const QueueFlags = QueueFamilyProperties.queueFlags;

    VkBool32 bPresentationSupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        PhysicalDevice, QueueFamilyIndex, m_VkSurface, &bPresentationSupported
    );
    if (!bPresentationSupported)
    {
        return 0;
    }

    if (QueueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        Score += 10;
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

void VulkanApp::LogPhysicalDeviceQueueFamiliesProperties(VkPhysicalDevice const PhysicalDevice) const
{
    std::vector<VkQueueFamilyProperties> QueueFamilesProperties =
        GetPhysicalDeviceQueueFamilyProperties(PhysicalDevice);

    uint32_t FamilyIndex = 0;
    for (VkQueueFamilyProperties const &QueueFamilyProperties : QueueFamilesProperties)
    {
        LogPhysicalDeviceQueueFamilyProperties(FamilyIndex++, QueueFamilyProperties);
    }
}

void VulkanApp::LogPhysicalDeviceQueueFamilyProperties(
    uint32_t const QueueFamilyIndex, VkQueueFamilyProperties const QueueFamilyProperties
) const
{
    VkBool32 bPresentationSupported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        m_VkPhysicalDevice, QueueFamilyIndex, m_VkSurface, &bPresentationSupported
    );

    VkQueueFlags const QueueFlags = QueueFamilyProperties.queueFlags;
    VKL_TRACE(
        "QueueFamily {}: QueueCount: {}, Bits: [Graphics: {:b}, Compute: {:b}, Transfer: {:b}, "
        "SparseBinding: {:b}], "
        "Presentation Support: {:b}",
        QueueFamilyIndex,
        QueueFamilyProperties.queueCount,
        static_cast<bool>(QueueFlags & VK_QUEUE_GRAPHICS_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_COMPUTE_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_TRANSFER_BIT),
        static_cast<bool>(QueueFlags & VK_QUEUE_SPARSE_BINDING_BIT),
        bPresentationSupported
    );
}

void VulkanApp::CreateDevice()
{
    QueueFamilyIndices PhysicalDeviceQueueFamilyIndices =
        GetPhysicalDeviceMostSuitableQueueFamilyIndices(m_VkPhysicalDevice);

    // clang-format off
    std::unordered_set<uint32_t> QueueFamilyIndices{
        PhysicalDeviceQueueFamilyIndices.GraphicsFamily.value(),
        PhysicalDeviceQueueFamilyIndices.PresentationFamily.value()
    };
    // clang-format on

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos(QueueFamilyIndices.size());

    float    QueuePriority = 1.0f;
    uint32_t Counter       = 0;
    for (uint32_t QueueFamilyIndex : QueueFamilyIndices)
    {
        VkDeviceQueueCreateInfo DeviceQueueCreateInfo{};
        DeviceQueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        DeviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
        DeviceQueueCreateInfo.queueCount       = 1;
        DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;

        QueueCreateInfos[Counter++] = DeviceQueueCreateInfo;
    }

    // Features supported by VkPhysicalDevice that are requested for use by VkDevice
    VkPhysicalDeviceFeatures DeviceRequestedFeatures{};

    std::vector<char const *> Extensions       = GetRequiredDeviceExtensions();
    std::vector<char const *> ValidationLayers = GetRequiredDeviceValidationLayers();

    VkDeviceCreateInfo DeviceCreateInfo{};
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.data();
    DeviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(QueueCreateInfos.size());
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
    VKL_TRACE("Created VkDevice successfully");
}

void VulkanApp::DestroyDevice()
{
    vkDestroyDevice(m_VkDevice, nullptr);
    VKL_TRACE("VkDevice destroyed");
}

std::vector<char const *> VulkanApp::GetRequiredDeviceExtensions() const
{
    // clang-format off
    std::vector<char const *> DeviceExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    // clang-format on
    return DeviceExtensions;
}

std::vector<char const *> VulkanApp::GetRequiredDeviceValidationLayers() const
{
    return GetRequiredInstanceValidationLayers(); // same as Instance Layers
}

void VulkanApp::RetrieveQueuesFromDevice()
{
    uint32_t QueueIndex = 0;
    vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.GraphicsFamily.value(), QueueIndex, &m_GraphicsQueue);
    vkGetDeviceQueue(
        m_VkDevice, m_QueueFamilyIndices.PresentationFamily.value(), QueueIndex, &m_PresentationQueue
    );

    VKL_TRACE("Retrieved VkQueues from VkDevice");
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
        VKL_TRACE("DebugCallback set up successfully");
    }
}

void VulkanApp::DestroyDebugCallback()
{
    Utils::DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);
    VKL_TRACE("DebugCallback destroyed");
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

void VulkanApp::CreateSurface()
{
    if (glfwCreateWindowSurface(m_VkInstance, m_Window.Get(), nullptr, &m_VkSurface) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkSurface!");
        exit(1);
    }
    VKL_TRACE("Created VkSurface successfully");
}

void VulkanApp::DestroySurface()
{
    vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);
    VKL_TRACE("VkSurface destroyed");
}

SwapchainSupportDetails VulkanApp::GetSwapchainSupportDetails(
    VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface
) const
{
    SwapchainSupportDetails SupportDetails;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SupportDetails.SurfaceCapabilities);

    uint32_t NumSurfaceFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &NumSurfaceFormats, nullptr);
    if (NumSurfaceFormats != 0)
    {
        SupportDetails.SurfaceFormats.resize(NumSurfaceFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            PhysicalDevice, Surface, &NumSurfaceFormats, SupportDetails.SurfaceFormats.data()
        );
    }

    uint32_t NumPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &NumPresentationModes, nullptr);
    if (NumPresentationModes != 0)
    {
        SupportDetails.PresentationMode.resize(NumPresentationModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice, Surface, &NumPresentationModes, SupportDetails.PresentationMode.data()
        );
    }

    return SupportDetails;
}

VkExtent2D VulkanApp::SelectSwapchainExtent(VkSurfaceCapabilitiesKHR const &Capabilities) const
{
    if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() &&
        Capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max())
    {
        return Capabilities.currentExtent; // swapchain images size == surface size
    }
    // else - surface size is determined by swapchain images size

    std::pair<int, int> WindowBufferPixelsSize = m_Window.GetFramebufferSize();

    VkExtent2D Extent = {
        static_cast<uint32_t>(WindowBufferPixelsSize.first),
        static_cast<uint32_t>(WindowBufferPixelsSize.second)};

    Extent.width =
        std::clamp(Extent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    Extent.height =
        std::clamp(Extent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

    return Extent;
}

VkSurfaceFormatKHR VulkanApp::SelectSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> const &Formats
) const
{
    for (VkSurfaceFormatKHR const &Format : Formats)
    {
        if (Format.format == VK_FORMAT_B8G8R8A8_SRGB && Format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            return Format; // most preferable
        }
    }
    return Formats[0];
}

VkPresentModeKHR VulkanApp::SelectSwapchainPresentationMode(std::vector<VkPresentModeKHR> const &Modes) const
{
    for (VkPresentModeKHR const &Mode : Modes)
    {
        if (Mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return Mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanApp::SelectSwapchainImagesCount(VkSurfaceCapabilitiesKHR const &Capabilities) const
{
    uint32_t ImagesCount = Capabilities.minImageCount + 1;

    if (Capabilities.maxImageCount != 0 && Capabilities.maxImageCount < ImagesCount)
    {                                             // maxImageCount == 0 -> count can be any value
        ImagesCount = Capabilities.maxImageCount; // do not exceed max count
    }

    return ImagesCount;
}

void VulkanApp::CreateSwapchain()
{
    SwapchainSupportDetails SupportDetails = GetSwapchainSupportDetails(m_VkPhysicalDevice, m_VkSurface);

    uint32_t const           ImagesCount = SelectSwapchainImagesCount(SupportDetails.SurfaceCapabilities);
    VkExtent2D const         Extent      = SelectSwapchainExtent(SupportDetails.SurfaceCapabilities);
    VkSurfaceFormatKHR const Format      = SelectSwapchainSurfaceFormat(SupportDetails.SurfaceFormats);
    VkPresentModeKHR const   PresentMode = SelectSwapchainPresentationMode(SupportDetails.PresentationMode);

    m_SwapchainExtent      = Extent;
    m_SwapchainImageFormat = Format.format;

    VkSwapchainCreateInfoKHR CreateInfo{};
    CreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    CreateInfo.surface          = m_VkSurface;
    CreateInfo.minImageCount    = ImagesCount;
    CreateInfo.imageExtent      = Extent;
    CreateInfo.imageFormat      = Format.format;
    CreateInfo.imageColorSpace  = Format.colorSpace;
    CreateInfo.presentMode      = PresentMode;
    CreateInfo.imageArrayLayers = 1; // Unless it's a stereoscopic 3D app
    CreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t QueueFamilyIndices[] = {
        m_QueueFamilyIndices.GraphicsFamily.value(), m_QueueFamilyIndices.PresentationFamily.value()};

    if (m_QueueFamilyIndices.GraphicsFamily.value() != m_QueueFamilyIndices.PresentationFamily.value())
    {
        CreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        CreateInfo.queueFamilyIndexCount = 2;
        CreateInfo.pQueueFamilyIndices   = QueueFamilyIndices;
    }
    else
    {
        CreateInfo.imageSharingMode =
            VK_SHARING_MODE_EXCLUSIVE;        // Images in swapchain will be used by one queue family only
        CreateInfo.queueFamilyIndexCount = 0; // Optional
        CreateInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    CreateInfo.preTransform   = SupportDetails.SurfaceCapabilities.currentTransform; // Don't want transforms
    CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                   // treat alpha as 1.0f
    CreateInfo.clipped        = VK_TRUE; // Ignore obstructed pixels
    CreateInfo.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_VkDevice, &CreateInfo, nullptr, &m_VkSwapchain) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkSwapchain!");
        exit(1);
    }
    VKL_TRACE("Created VkSwapchain successfully");
}

void VulkanApp::DestroySwapchain()
{
    vkDestroySwapchainKHR(m_VkDevice, m_VkSwapchain, nullptr);
    VKL_TRACE("VkSwapchain destroyed");
}

void VulkanApp::RetrieveSwapchainImages()
{
    uint32_t SwapchainImagesCount = 0;
    vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchain, &SwapchainImagesCount, nullptr);

    m_SwapchainImages.resize(SwapchainImagesCount);
    vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapchain, &SwapchainImagesCount, m_SwapchainImages.data());

    VKL_TRACE("Retrieved VkImages from VkSwapchain");
}

void VulkanApp::CreateSwapchainImagesViews()
{
    m_SwapchainImagesViews.resize(m_SwapchainImages.size());
    for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo CreateInfo{};
        CreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image    = m_SwapchainImages[i];
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        CreateInfo.format   = m_SwapchainImageFormat;

        CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Default mapping
        CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        CreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        CreateInfo.subresourceRange.baseMipLevel   = 0;
        CreateInfo.subresourceRange.levelCount     = 1;
        CreateInfo.subresourceRange.baseArrayLayer = 0;
        CreateInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(m_VkDevice, &CreateInfo, nullptr, &m_SwapchainImagesViews[i]) != VK_SUCCESS)
        {
            VKL_CRITICAL("Failed to create VkImageView!");
            exit(1);
        }
    }
    VKL_TRACE("Created VkImageViews successfully");
}

void VulkanApp::DestroySwapchainImagesViews()
{
    for (VkImageView const &ImageView : m_SwapchainImagesViews)
    {
        vkDestroyImageView(m_VkDevice, ImageView, nullptr);
    }
    VKL_TRACE("VkImageViews destroyed");
}

void VulkanApp::CreateRenderPass()
{
    VkAttachmentDescription ColorAttachment{};
    ColorAttachment.format         = m_SwapchainImageFormat;
    ColorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ColorAttachmentRef{};
    ColorAttachmentRef.attachment = 0; // index of attachment in pAttachments array in RenderPassInfo
    ColorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Subpass{};
    Subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments    = &ColorAttachmentRef;

    VkSubpassDependency Dependency{};
    Dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    Dependency.dstSubpass    = 0; // our only subpass
    Dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.srcAccessMask = 0;
    Dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassInfo{};
    RenderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = 1;
    RenderPassInfo.pAttachments    = &ColorAttachment; // index = .frag output with layout(location = 0)
    RenderPassInfo.subpassCount    = 1;
    RenderPassInfo.pSubpasses      = &Subpass;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies   = &Dependency;

    if (vkCreateRenderPass(m_VkDevice, &RenderPassInfo, nullptr, &m_VkRenderPass) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkRenderPass!");
        exit(1);
    }
    VKL_TRACE("Created VkRenderPass successfully");
}

void VulkanApp::DestroyRenderPass()
{
    vkDestroyRenderPass(m_VkDevice, m_VkRenderPass, nullptr);
    VKL_TRACE("VkRenderPass destroyed");
}

VkPipelineShaderStageCreateInfo VulkanApp::GetVertexShaderStageInfo(VkShaderModule ShaderModule) const
{
    VkPipelineShaderStageCreateInfo VertexShaderStageInfo{};
    VertexShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertexShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    VertexShaderStageInfo.module = ShaderModule;
    VertexShaderStageInfo.pName  = "main";
    // VertexShaderStageInfo.pSpecializationInfo = nullptr; for configuration variables

    return VertexShaderStageInfo;
}

VkPipelineShaderStageCreateInfo VulkanApp::GetFragmentShaderStageInfo(VkShaderModule ShaderModule) const
{
    VkPipelineShaderStageCreateInfo FragmentShaderStageInfo{};
    FragmentShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragmentShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragmentShaderStageInfo.module = ShaderModule;
    FragmentShaderStageInfo.pName  = "main";

    return FragmentShaderStageInfo;
}

VkPipelineVertexInputStateCreateInfo VulkanApp::GetVertexInputStateInfo() const
{
    VkPipelineVertexInputStateCreateInfo VertexInputStageInfo{};
    VertexInputStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputStageInfo.vertexBindingDescriptionCount   = 0; // Zeroes bcz vertex info hardcoded in shader
    VertexInputStageInfo.pVertexBindingDescriptions      = nullptr;
    VertexInputStageInfo.vertexAttributeDescriptionCount = 0;
    VertexInputStageInfo.pVertexAttributeDescriptions    = nullptr;

    return VertexInputStageInfo;
}

VkPipelineInputAssemblyStateCreateInfo VulkanApp::GetInputAssemblyStateInfo() const
{
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyStageInfo{};
    InputAssemblyStageInfo.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyStageInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyStageInfo.primitiveRestartEnable = VK_FALSE;

    return InputAssemblyStageInfo;
}

VkViewport VulkanApp::GetStaticViewportInfo() const
{
    VkViewport Viewport{};
    Viewport.x        = 0.0f;
    Viewport.y        = 0.0f;
    Viewport.width    = static_cast<float>(m_SwapchainExtent.width);
    Viewport.height   = static_cast<float>(m_SwapchainExtent.height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
    return Viewport;
}

VkRect2D VulkanApp::GetStaticScissorInfo() const
{
    VkRect2D Scissor{};
    Scissor.offset = {0, 0};
    Scissor.extent = m_SwapchainExtent;
    return Scissor;
}

std::pair<VkPipelineDynamicStateCreateInfo, std::vector<VkDynamicState>> VulkanApp::GetDynamicStateInfo(
) const
{
    std::vector<VkDynamicState>      DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo DynamicStateInfo{};
    DynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
    DynamicStateInfo.pDynamicStates    = DynamicStates.data();

    return {DynamicStateInfo, std::move(DynamicStates)};
}

VkPipelineViewportStateCreateInfo VulkanApp::GetStaticViewportStateInfo(
    VkViewport const *Viewport, VkRect2D const *Scissor
) const
{
    VkPipelineViewportStateCreateInfo ViewportState{};
    ViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.pViewports    = Viewport;
    ViewportState.scissorCount  = 1;
    ViewportState.pScissors     = Scissor;
    return ViewportState;
}

VkPipelineViewportStateCreateInfo VulkanApp::GetDynamicViewportStateInfo() const
{
    VkPipelineViewportStateCreateInfo ViewportState{};
    ViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.pViewports    = nullptr; // ignored if dynamic
    ViewportState.scissorCount  = 1;
    ViewportState.pScissors     = nullptr; // ignored if dynamic
    return ViewportState;
}

VkPipelineRasterizationStateCreateInfo VulkanApp::GetRasterizerStateInfo() const
{
    VkPipelineRasterizationStateCreateInfo RasterizerStageInfo{};
    RasterizerStageInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerStageInfo.depthClampEnable        = VK_FALSE;
    RasterizerStageInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerStageInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    RasterizerStageInfo.lineWidth               = 1.0f;
    RasterizerStageInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    RasterizerStageInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizerStageInfo.depthBiasEnable         = VK_FALSE;
    RasterizerStageInfo.depthBiasConstantFactor = 0.0f; // optional
    RasterizerStageInfo.depthBiasClamp          = 0.0f; // optional
    RasterizerStageInfo.depthBiasSlopeFactor    = 0.0f; // optional

    return RasterizerStageInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanApp::GetMultisamplerStateInfo() const
{
    VkPipelineMultisampleStateCreateInfo MultisamplerStateInfo{};
    MultisamplerStateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisamplerStateInfo.sampleShadingEnable   = VK_FALSE;
    MultisamplerStateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MultisamplerStateInfo.minSampleShading      = 1.0f;     // optional
    MultisamplerStateInfo.pSampleMask           = nullptr;  // optional
    MultisamplerStateInfo.alphaToCoverageEnable = VK_FALSE; // optional
    MultisamplerStateInfo.alphaToOneEnable      = VK_FALSE; // optional

    return MultisamplerStateInfo;
}

VkPipelineColorBlendAttachmentState VulkanApp::GetColorBlendAttachment() const
{
    VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable         = VK_FALSE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // optional
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    ColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;      // optional
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // optional
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    ColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // optional

    return ColorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo VulkanApp::GetColorBlendStateInfo(
    VkPipelineColorBlendAttachmentState const *ColorBlendAttachment
) const
{
    VkPipelineColorBlendStateCreateInfo ColorBlendState{};
    ColorBlendState.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.logicOpEnable     = VK_FALSE;
    ColorBlendState.logicOp           = VK_LOGIC_OP_COPY; // optional
    ColorBlendState.attachmentCount   = 1;
    ColorBlendState.pAttachments      = ColorBlendAttachment;
    ColorBlendState.blendConstants[0] = 0.0f; // optional
    ColorBlendState.blendConstants[1] = 0.0f; // optional
    ColorBlendState.blendConstants[2] = 0.0f; // optional
    ColorBlendState.blendConstants[3] = 0.0f; // optional

    return ColorBlendState;
}

void VulkanApp::CreatePipelineLayout()
{
    VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
    PipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount         = 0;       // optional
    PipelineLayoutInfo.pSetLayouts            = nullptr; // optional
    PipelineLayoutInfo.pushConstantRangeCount = 0;       // optional
    PipelineLayoutInfo.pPushConstantRanges    = nullptr; // optional

    if (vkCreatePipelineLayout(m_VkDevice, &PipelineLayoutInfo, nullptr, &m_VkPipelineLayout) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkPipelineLayout!");
        exit(1);
    }
    VKL_TRACE("Created VkPipelineLayout successfully");
}

void VulkanApp::DestroyPipelineLayout()
{
    vkDestroyPipelineLayout(m_VkDevice, m_VkPipelineLayout, nullptr);
    VKL_TRACE("VkPipelineLayout destroyed");
}

void VulkanApp::CreatePipeline()
{
    // Programmable stages
    std::vector<char> VertexShaderByteCode   = ReadSPIRVByteCode("./Assets/Shaders/vert.spv");
    std::vector<char> FragmentShaderByteCode = ReadSPIRVByteCode("./Assets/Shaders/frag.spv");

    VkShaderModule VertexShaderModule   = CreateShaderModule(VertexShaderByteCode);
    VkShaderModule FragmentShaderModule = CreateShaderModule(FragmentShaderByteCode);

    VkPipelineShaderStageCreateInfo VertexShaderStageInfo = GetVertexShaderStageInfo(VertexShaderModule);
    VkPipelineShaderStageCreateInfo FragmentShaderStageInfo =
        GetFragmentShaderStageInfo(FragmentShaderModule);

    VkPipelineShaderStageCreateInfo ShaderStagesInfo[] = {VertexShaderStageInfo, FragmentShaderStageInfo};

    // Fixed stages
    VkPipelineVertexInputStateCreateInfo   VertexInputStageInfo   = GetVertexInputStateInfo();
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyStageInfo = GetInputAssemblyStateInfo();

    // Dynamic Viewport and Scissor
    auto [DynamicStateInfo, DynamicStates] = GetDynamicStateInfo(); // TODO: check reference being correct!!

    VkPipelineViewportStateCreateInfo ViewportState = GetDynamicViewportStateInfo();

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo RasterizerStageInfo = GetRasterizerStateInfo();

    // Multisampling
    VkPipelineMultisampleStateCreateInfo MultisamplerStateInfo = GetMultisamplerStateInfo();

    // Depth and Stencil tests
    // Not needed - pass nullptr

    // Color blending
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = GetColorBlendAttachment();
    VkPipelineColorBlendStateCreateInfo ColorBlendState      = GetColorBlendStateInfo(&ColorBlendAttachment);

    // Pipeline creation
    VkGraphicsPipelineCreateInfo PipelineCreateInfo{};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Programmable stages
    PipelineCreateInfo.stageCount = 2;
    PipelineCreateInfo.pStages    = ShaderStagesInfo;

    // Fixed stages
    PipelineCreateInfo.pVertexInputState   = &VertexInputStageInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssemblyStageInfo;
    PipelineCreateInfo.pDynamicState       = &DynamicStateInfo;
    PipelineCreateInfo.pViewportState      = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &RasterizerStageInfo;
    PipelineCreateInfo.pMultisampleState   = &MultisamplerStateInfo;
    PipelineCreateInfo.pDepthStencilState  = nullptr;
    PipelineCreateInfo.pColorBlendState    = &ColorBlendState;

    // Uniforms and push-constants specified in layout
    PipelineCreateInfo.layout = m_VkPipelineLayout;

    // RenderPass and it's Subpass in which Pipeline is used
    PipelineCreateInfo.renderPass = m_VkRenderPass;
    PipelineCreateInfo.subpass    = 0;

    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex  = -1;

    VkResult PipelineCreateResult =
        vkCreateGraphicsPipelines(m_VkDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &m_VkPipeline);

    DestroyShaderModule(FragmentShaderModule);
    DestroyShaderModule(VertexShaderModule);

    if (PipelineCreateResult != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkPipeline!");
        exit(1);
    }
    VKL_TRACE("Created VkPipeline successfully");
}

void VulkanApp::DestroyPipeline()
{
    vkDestroyPipeline(m_VkDevice, m_VkPipeline, nullptr);
    VKL_TRACE("VkPipeline destroyed");
}

std::vector<char> VulkanApp::ReadSPIRVByteCode(std::filesystem::path const &FilePath) const
{
    std::ifstream File(FilePath, std::ios::ate | std::ios::binary);

    size_t            FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> ByteCode(FileSize);

    File.seekg(0);
    File.read(ByteCode.data(), FileSize);

    return ByteCode;
}

VkShaderModule VulkanApp::CreateShaderModule(std::vector<char> const &SPIRVByteCode) const
{
    VkShaderModuleCreateInfo CreateInfo{};

    CreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = SPIRVByteCode.size();
    CreateInfo.pCode    = reinterpret_cast<uint32_t const *>(SPIRVByteCode.data());

    VkShaderModule ShaderModule{};
    if (vkCreateShaderModule(m_VkDevice, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkShaderModule!");
        exit(1);
    }
    return ShaderModule;
}

void VulkanApp::DestroyShaderModule(VkShaderModule ShaderModule) const
{
    vkDestroyShaderModule(m_VkDevice, ShaderModule, nullptr);
}

void VulkanApp::CreateFramebuffer()
{
    m_VkFramebuffers.resize(m_SwapchainImagesViews.size());

    for (size_t i = 0; i < m_VkFramebuffers.size(); ++i)
    {
        VkImageView Attachments[] = {m_SwapchainImagesViews[i]};

        VkFramebufferCreateInfo FramebufferInfo{};
        FramebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass      = m_VkRenderPass;
        FramebufferInfo.attachmentCount = 1;
        FramebufferInfo.pAttachments    = Attachments;
        FramebufferInfo.width           = m_SwapchainExtent.width;
        FramebufferInfo.height          = m_SwapchainExtent.height;
        FramebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(m_VkDevice, &FramebufferInfo, nullptr, &m_VkFramebuffers[i]) != VK_SUCCESS)
        {
            VKL_CRITICAL("Failed to create VkFramebuffer!");
            exit(1);
        }
    }
    VKL_TRACE("Created VkFramebuffers successfully");
}

void VulkanApp::DestroyFramebuffer()
{
    for (size_t i = 0; i < m_VkFramebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(m_VkDevice, m_VkFramebuffers[i], nullptr);
    }
    VKL_TRACE("VkFramebuffers destroyed");
}

void VulkanApp::CreateCommandPool()
{
    VkCommandPoolCreateInfo CommandPoolInfo{};
    CommandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CommandPoolInfo.queueFamilyIndex = m_QueueFamilyIndices.GraphicsFamily.value();

    if (vkCreateCommandPool(m_VkDevice, &CommandPoolInfo, nullptr, &m_VkCommandPool) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to create VkCommandPool!");
        exit(1);
    }
    VKL_TRACE("Created VkCommandPool successfully");
}

void VulkanApp::DestroyCommandPool()
{
    vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);
    VKL_TRACE("VkCommandPool destroyed");
}

void VulkanApp::AllocateCommandBuffers()
{
    VkCommandBufferAllocateInfo CommandBufferInfo{};
    CommandBufferInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferInfo.commandPool        = m_VkCommandPool;
    CommandBufferInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferInfo.commandBufferCount = static_cast<uint32_t>(m_VkCommandBuffers.size());

    if (vkAllocateCommandBuffers(m_VkDevice, &CommandBufferInfo, m_VkCommandBuffers.data()) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to allocate VkCommandBuffer!");
        exit(1);
    }
    VKL_TRACE("Allocated VkCommandBuffer successfully");
}

void VulkanApp::RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint32_t SwapchainImageIndex)
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo{};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to Begin VkCommandBuffer!");
        exit(1);
    }

    VkRenderPassBeginInfo RenderPassBeginInfo{};
    RenderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass        = m_VkRenderPass;
    RenderPassBeginInfo.framebuffer       = m_VkFramebuffers[SwapchainImageIndex];
    RenderPassBeginInfo.renderArea.offset = {0, 0};
    RenderPassBeginInfo.renderArea.extent = m_SwapchainExtent;

    VkClearValue ClearColor             = {0.0f, 0.0f, 0.0f, 1.0f};
    RenderPassBeginInfo.clearValueCount = 1;
    RenderPassBeginInfo.pClearValues    = &ClearColor;

    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipeline);

        // Viewport and Scissor are dynamic - specify them here
        VkViewport Viewport{};
        Viewport.x        = 0.0f;
        Viewport.y        = 0.0f;
        Viewport.width    = static_cast<float>(m_SwapchainExtent.width);
        Viewport.height   = static_cast<float>(m_SwapchainExtent.height);
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;
        vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

        VkRect2D Scissor{};
        Scissor.offset = {0, 0};
        Scissor.extent = m_SwapchainExtent;
        vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

        vkCmdDraw(CommandBuffer, 3, 1, 0, 0);
    }
    vkCmdEndRenderPass(CommandBuffer);

    if (vkEndCommandBuffer(CommandBuffer) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to End VkCommandBuffer!");
        exit(1);
    }
}

void VulkanApp::SubmitCommandBuffer(VkCommandBuffer CommandBuffer)
{
    VkSemaphore          WaitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
    VkPipelineStageFlags WaitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore SignalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount   = 1;
    SubmitInfo.pWaitSemaphores      = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask    = WaitStages;
    SubmitInfo.commandBufferCount   = 1;
    SubmitInfo.pCommandBuffers      = &CommandBuffer;
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores    = SignalSemaphores;

    if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
    {
        VKL_CRITICAL("Failed to submit draw commands buffer!");
        exit(1);
    }
}

void VulkanApp::PresentResult(uint32_t SwapchainImageIndex)
{
    VkSwapchainKHR Swapchains[] = {m_VkSwapchain};

    VkSemaphore WaitSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

    VkPresentInfoKHR PresentInfo{};
    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores    = WaitSemaphores;
    PresentInfo.swapchainCount     = 1;
    PresentInfo.pSwapchains        = Swapchains;
    PresentInfo.pImageIndices      = &SwapchainImageIndex;
    PresentInfo.pResults           = nullptr;

    vkQueuePresentKHR(m_GraphicsQueue, &PresentInfo);
}

void VulkanApp::CreateSyncObjects()
{
    for (uint32_t i = 0; i < s_FramesInFlight; ++i)
    {
        VkSemaphoreCreateInfo ImageAvailableSemaphoreInfo{};
        ImageAvailableSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphoreCreateInfo RenderFinishedSemaphoreInfo{};
        RenderFinishedSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo InFlightFenceInfo{};
        InFlightFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        InFlightFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(
                m_VkDevice, &ImageAvailableSemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]
            ) != VK_SUCCESS ||
            vkCreateSemaphore(
                m_VkDevice, &RenderFinishedSemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]
            ) != VK_SUCCESS ||
            vkCreateFence(m_VkDevice, &InFlightFenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
        {
            VKL_CRITICAL("Failed to create Syncronization objects!");
            exit(1);
        }
    }
    VKL_TRACE("Created Syncronization objects successfully");
}

void VulkanApp::DestroySyncObjects()
{
    for (uint32_t i = 0; i < s_FramesInFlight; ++i)
    {
        vkDestroySemaphore(m_VkDevice, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_VkDevice, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_VkDevice, m_InFlightFences[i], nullptr);
    }
    VKL_TRACE("Syncronization objects destroyed");
}
