#ifndef VULKANLEARNING_VULKANAPP
#define VULKANLEARNING_VULKANAPP

#include "Camera.h"
#include "Log.h"
#include "QueueFamilyIndices.h"
#include "SwapchainSupportDetails.h"
#include "Vertex.h"
#include "Window.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

// Vulkan Header
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

    void UpdateCamera(float ElapsedTime);

    void DrawFrame();

private:
    Window m_Window;

    uint32_t                  m_CurrentFrame   = 0;
    static constexpr uint32_t s_FramesInFlight = 2;

    // VK_ERROR_OUT_OF_DATE_KHR not guaranteed
    bool m_bWindowResizeHappened = false;

    static void OnWindowResized(GLFWwindow *Window, int NewWidth, int NewHeight);

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
    // !VK_INSTANCE_RELATED
    //=========================================================================================================
    // VK_PHYSICAL_DEVICE
    void SelectPhysicalDevice();

    std::vector<VkPhysicalDevice>      GetPhysicalDevices() const;
    VkPhysicalDeviceProperties         GetPhysicalDeviceProperties(VkPhysicalDevice PhysicalDevice) const;
    VkPhysicalDeviceFeatures           GetPhysicalDeviceFeatures(VkPhysicalDevice PhysicalDevice) const;
    std::vector<VkExtensionProperties> GetPhysicalDeviceSupportedExtensions(VkPhysicalDevice PhysicalDevice
    ) const;

    VkPhysicalDevice GetMostSuitablePhysicalDevice(std::vector<VkPhysicalDevice> const &PhysicalDevices
    ) const;

    bool     IsPhysicalDeviceSuitable(VkPhysicalDevice PhysicalDevice) const;
    bool     IsPhysicalDeviceExtensionSupportComplete(VkPhysicalDevice PhysicalDevice) const;
    uint32_t GetPhysicalDeviceSuitability(VkPhysicalDevice PhysicalDevice) const;
    // !VK_PHYSICAL_DEVICE
    //=========================================================================================================
    // VK_QUEUE_FAMILY
    std::vector<VkQueueFamilyProperties> GetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice PhysicalDevice
    ) const;

    QueueFamilyIndices GetPhysicalDeviceMostSuitableQueueFamilyIndices(VkPhysicalDevice PhysicalDevice) const;

    std::optional<uint32_t> GetPhysicalDeviceMostSuitableQueueFamily(
        VkPhysicalDevice                            PhysicalDevice,
        std::vector<VkQueueFamilyProperties> const &QueueFamiliesProperties,
        uint32_t (VulkanApp::*SuitabilityCalculatorFunction
        )(VkPhysicalDevice const, VkQueueFamilyProperties const &, uint32_t) const
    ) const;
    uint32_t GetPhysicalDeviceGraphicsQueueFamilySuitability(
        VkPhysicalDevice               PhysicalDevice,
        VkQueueFamilyProperties const &QueueFamilyProperties,
        uint32_t                       QueueFamilyIndex
    ) const;
    uint32_t GetPhysicalDevicePresentationQueueFamilySuitability(
        VkPhysicalDevice               PhysicalDevice,
        VkQueueFamilyProperties const &QueueFamilyProperties,
        uint32_t                       QueueFamilyIndex
    ) const;
    // !VK_QUEUE_FAMILY
    //=========================================================================================================
    // VK_DEVICE
    void CreateDevice();
    void DestroyDevice();

    std::vector<char const *> GetRequiredDeviceExtensions() const;
    std::vector<char const *> GetRequiredDeviceValidationLayers(
    ) const; // Ignored by newer Vulkan versions and uses Layers from VkInstance, left for compatibility

    void RetrieveQueuesFromDevice();
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
    //=========================================================================================================
    // VK_KHR_SURFACE
    void CreateSurface();
    void DestroySurface();
    // !VK_KHR_SURFACE
    //=========================================================================================================
    // VK_KHR_SWAPCHAIN
    SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
        const;

    VkExtent2D         SelectSwapchainExtent(VkSurfaceCapabilitiesKHR const &Capabilities) const;
    VkSurfaceFormatKHR SelectSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> const &Formats) const;
    VkPresentModeKHR   SelectSwapchainPresentationMode(std::vector<VkPresentModeKHR> const &Modes) const;
    uint32_t           SelectSwapchainImagesCount(VkSurfaceCapabilitiesKHR const &Capabilities) const;

    void CreateSwapchain();
    void DestroySwapchain();

    void RecreateSwapchain();

    void RetrieveSwapchainImages();
    // !VK_KHR_SWAPCHAIN
    //=========================================================================================================
    // VK_IMAGE_VIEW
    void CreateSwapchainImagesViews();
    void DestroySwapchainImagesViews();
    // !VK_IMAGE_VIEW
    //=========================================================================================================
    // VK_PIPELINE

    // Render Pass
    void CreateRenderPass();
    void DestroyRenderPass();

    // Programmable Stages
    VkPipelineShaderStageCreateInfo GetVertexShaderStageInfo(VkShaderModule ShaderModule) const;
    VkPipelineShaderStageCreateInfo GetFragmentShaderStageInfo(VkShaderModule ShaderModule) const;

    // Fixed Stages
    VkPipelineVertexInputStateCreateInfo   GetVertexInputStateInfo();
    VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateInfo() const;

    // Static Viewport and Scissor
    VkViewport GetStaticViewportInfo() const;
    VkRect2D   GetStaticScissorInfo() const;

    // Dynamic Viewport and Scissors
    std::pair<VkPipelineDynamicStateCreateInfo, std::vector<VkDynamicState>> GetDynamicStateInfo() const;

    // Viewport
    VkPipelineViewportStateCreateInfo GetStaticViewportStateInfo(
        VkViewport const *Viewport, VkRect2D const *Scissor
    ) const;
    VkPipelineViewportStateCreateInfo GetDynamicViewportStateInfo() const;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo GetRasterizerStateInfo() const;

    // Multisampler
    VkPipelineMultisampleStateCreateInfo GetMultisamplerStateInfo() const;

    // Depth and Stencil Tests
    // Not needed here

    // Color Blending
    VkPipelineColorBlendAttachmentState GetColorBlendAttachment() const;
    VkPipelineColorBlendStateCreateInfo GetColorBlendStateInfo(
        VkPipelineColorBlendAttachmentState const *ColorBlendAttachment
    ) const;

    // Pipeline layout
    void CreatePipelineLayout();
    void DestroyPipelineLayout();

    void CreatePipeline();
    void DestroyPipeline();
    // !VK_PIPELINE
    //=========================================================================================================
    // VK_SPIRV_SHADER
    std::vector<char> ReadSPIRVByteCode(std::filesystem::path const &FilePath) const;

    VkShaderModule CreateShaderModule(std::vector<char> const &SPIRVByteCode) const;
    void           DestroyShaderModule(VkShaderModule ShaderModule) const;
    // !VK_SPIRV_SHADER
    //=========================================================================================================
    // VK_FRAMEBUFFER
    void CreateFramebuffers();
    void DestroyFramebuffers();
    // !VK_FRAMEBUFFER
    //=========================================================================================================
    // VK_BUFFER
    void CreateBuffer(
        VkBuffer             &Buffer,
        VkDeviceMemory       &BufferMemory,
        VkBufferUsageFlags    Usage,
        VkDeviceSize          Size,
        VkMemoryPropertyFlags Properties
    );
    void DestroyBuffer(VkBuffer &Buffer, VkDeviceMemory &BufferMemory);

    void CreateVertexBuffer();
    void DestroyVertexBuffer();

    void CreateIndexBuffer();
    void DestroyIndexBuffer();

    void TransferBufferData(VkBuffer Source, VkBuffer Destination, VkDeviceSize Size);
    // !VK_BUFFER
    //=========================================================================================================
    // VK_DESCRIPTOR
    void CreateDescriptorSetLayout();
    void DestroyDescriptorSetLayout();

    void CreateUniformBuffers();
    void DestroyUniformBuffers();

    void UpdateUniformBuffers();

    void CreateDescriptorPool();
    void DestroyDescriptorPool();

    void AllocateDescriptorSets();
    // !VK_DESCRIPTOR
    //=========================================================================================================
    // VK_COMMAND_BUFFER
    void CreateCommandPool();
    void DestroyCommandPool();

    void AllocateCommandBuffers();

    void     RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint32_t SwapchainImageIndex);
    void     SubmitCommandBuffer(VkCommandBuffer CommandBuffer);
    VkResult PresentResult(uint32_t SwapchainImageIndex);
    // !VK_COMMAND_BUFFER
    //=========================================================================================================
    // VK_SYNC
    void CreateSyncObjects();
    void DestroySyncObjects();
    // !VK_SYNC

private:
    VkInstance m_VkInstance{};

    VkPhysicalDevice   m_VkPhysicalDevice{};
    QueueFamilyIndices m_QueueFamilyIndices{};

    VkDevice m_VkDevice{};
    VkQueue  m_VkGraphicsQueue{};
    VkQueue  m_VkPresentationQueue{};

    VkSurfaceKHR             m_VkSurface{};
    VkSwapchainKHR           m_VkSwapchain{};
    VkExtent2D               m_SwapchainExtent{};
    VkFormat                 m_SwapchainImageFormat{};
    std::vector<VkImage>     m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImagesViews;

    VkDescriptorSetLayout                        m_VkMatricesUBOLayout;
    std::array<VkBuffer, s_FramesInFlight>       m_VkMatricesUBOs;
    std::array<VkDeviceMemory, s_FramesInFlight> m_VkMatricesUBOsMemory;
    std::array<void *, s_FramesInFlight>         m_MatricesUBOsMappedMemory;

    VkDescriptorPool                              m_VkDescriptorPool;
    std::array<VkDescriptorSet, s_FramesInFlight> m_VkDescriptorSets;

    VkRenderPass     m_VkRenderPass{};
    VkPipelineLayout m_VkPipelineLayout{};
    VkPipeline       m_VkPipeline{};

    std::vector<VkFramebuffer> m_VkFramebuffers;

    std::vector<Vertex>                              m_Vertices;
    VkVertexInputBindingDescription                  m_VertexInputBindingDescription;
    std::array<VkVertexInputAttributeDescription, 2> m_VertexInputAttributeDescriptions;
    VkBuffer                                         m_VkVertexBuffer;
    VkDeviceMemory                                   m_VkVertexBufferMemory;

    std::vector<uint16_t> m_Indices;
    VkBuffer              m_VkIndexBuffer;
    VkDeviceMemory        m_VkIndexBufferMemory;

    VkCommandPool m_VkCommandPool{};
    VkCommandPool m_VkTransferCommandPool;

    std::array<VkCommandBuffer, s_FramesInFlight> m_VkCommandBuffers{};

    std::array<VkSemaphore, s_FramesInFlight> m_ImageAvailableSemaphores{};
    std::array<VkSemaphore, s_FramesInFlight> m_RenderFinishedSemaphores{};
    std::array<VkFence, s_FramesInFlight>     m_InFlightFences{};

    VkDebugUtilsMessengerEXT m_VkDebugMessenger{};

    Camera    m_Camera{};
    glm::vec2 m_CursorPos{};
};

#endif // !VULKANLEARNING_VULKANAPP
