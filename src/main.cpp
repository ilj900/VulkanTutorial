#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>

using uint = std::uint32_t;

const uint WIDTH = 1920;
const uint HEIGHT = 1080;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef NDEBUG
const bool bEnableValidationLayers = true;
#else
const bool bEnableValidationLayers = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallBackData,
        void* pUserData)
{
    std::cerr << "Validation layer: " << pCallBackData->pMessage << std::endl;

    return VK_FALSE;
}

static std::vector<char> ReadFile(const std::string& FileName)
{
    std::ifstream File(FileName, std::ios::ate | std::ios::binary);

    if (!File.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    std::size_t FileSize = (std::size_t)File.tellg();
    std::vector<char> Buffer(FileSize);

    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();

    return Buffer;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo, const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger)
{
    auto Function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    if (Function != nullptr)
    {
        return Function(Instance, CreateInfo, Allocator, DebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, const VkAllocationCallbacks* Allocator)
{
    auto Function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Function != nullptr)
    {
        Function(Instance, DebugMessenger, Allocator);
    }
}

class FHelloTriangleApplication
{
public:
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    struct QueueFamilyIndices
    {
        std::optional<uint> GraphicsFamily;
        std::optional<uint> PresentFamily;

        bool IsComplete()
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    std::vector<const char*> GetRequestedExtensions()
    {
        uint GLFWExtensionCount = 0;
        const char** GLFWExtensions;

        GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionCount);

        std::vector<const char*> Extensions(GLFWExtensions, GLFWExtensions + GLFWExtensionCount);

        if (bEnableValidationLayers)
        {
            Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return Extensions;
    }

    bool CheckValidationLayersSupport()
    {
        uint LayerCount;
        vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

        std::vector<VkLayerProperties> AvailableLayers(LayerCount);
        vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

        for (const char* LayerName : ValidationLayers)
        {
            bool LayerFound = false;

            for (const auto& LayerPropertys : AvailableLayers)
            {
                if (strcmp(LayerName, LayerPropertys.layerName) == 0)
                {
                    LayerFound = true;
                    break;
                }
            }
            if (!LayerFound)
            {
                return false;
            }
        }

        return true;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice Device)
    {
        uint ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr);

        std::vector<VkExtensionProperties>AvailableExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data());

        std::set<std::string> RequiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

        for (const auto& Extension : AvailableExtensions)
        {
            RequiredExtensions.erase(Extension.extensionName);
        }

        return RequiredExtensions.empty();

    }

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(Window, this);
        glfwSetFramebufferSizeCallback(Window, FramebufferResizeCallback);
    }

    static void FramebufferResizeCallback(GLFWwindow* Window, int Width, int Height)
    {
        auto App = reinterpret_cast<FHelloTriangleApplication*>(glfwGetWindowUserPointer(Window));
        App->bFramebufferResized = true;
    }

    void CreateInstance()
    {
        if (bEnableValidationLayers && !CheckValidationLayersSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }
        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = "Hello Triangle";
        AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;

        auto Extensions = GetRequestedExtensions();

        CreateInfo.enabledExtensionCount = static_cast<uint>(Extensions.size());
        CreateInfo.ppEnabledExtensionNames = Extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo;
        if (bEnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();

            PopulateDebugMessengerCreateInfo(DebugCreateInfo);
            CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &DebugCreateInfo;
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;

            CreateInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create instance!");
        }
    }

    SwapChainSupportDetails QuerrySwapChainSupport(VkPhysicalDevice Device)
    {
        SwapChainSupportDetails Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Details.Capabilities);

        uint FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, nullptr);
        if (FormatCount != 0)
        {
            Details.Formats.resize(FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &FormatCount, Details.Formats.data());
        }

        uint PresentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, nullptr);

        if (PresentModeCount != 0)
        {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &PresentModeCount, Details.PresentModes.data());
        }

        return Details;
    }

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
    {
        CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        CreateInfo.pfnUserCallback = DebugCallback;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& AvailableFormats)
    {
        for (const auto& AvailableFormat : AvailableFormats)
        {
            if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return AvailableFormat;
            }
        }

        return AvailableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& AvailablePresentModes)
    {

        for (const auto& AvailablePresentMode : AvailablePresentModes)
        {
            if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return AvailablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities)
    {
        if (Capabilities.currentExtent.width != UINT32_MAX)
        {
            return Capabilities.currentExtent;
        }
        else
        {
            int Width, Height;

            glfwGetFramebufferSize(Window, &Width, &Height);

            VkExtent2D ActualExtent = {static_cast<uint>(Width), static_cast<uint>(Height)};
            ActualExtent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.minImageExtent.width, ActualExtent.width));
            ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.minImageExtent.height, ActualExtent.height));
            return ActualExtent;
        }
    }

    void CreateSwapChain()
    {
        SwapChainSupportDetails SwapChainSupport = QuerrySwapChainSupport(PhysicalDevice);

        VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapChainSupport.Formats);
        VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapChainSupport.PresentModes);
        VkExtent2D Extent = ChooseSwapExtent(SwapChainSupport.Capabilities);

        uint ImageCount = SwapChainSupport.Capabilities.minImageCount + 1;

        if (SwapChainSupport.Capabilities.maxImageCount > 0 && ImageCount > SwapChainSupport.Capabilities.maxImageCount)
        {
            ImageCount = SwapChainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        CreateInfo.surface = Surface;

        CreateInfo.minImageCount = ImageCount;
        CreateInfo.imageFormat = SurfaceFormat.format;
        CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        CreateInfo.imageExtent = Extent;
        CreateInfo.imageArrayLayers = 1;
        CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);
        uint QueueFamilyIndices[] = {Indices.GraphicsFamily.value(), Indices.PresentFamily.value()};

        if (Indices.GraphicsFamily != Indices.PresentFamily)
        {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            CreateInfo.queueFamilyIndexCount = 2;
            CreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
        }
        else
        {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            CreateInfo.queueFamilyIndexCount = 0;
            CreateInfo.pQueueFamilyIndices = nullptr;
        }

        CreateInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
        CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        CreateInfo.presentMode = PresentMode;
        CreateInfo.clipped = VK_TRUE;

        CreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(Device, &CreateInfo, nullptr, &SwapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(Device, SwapChain, &ImageCount, nullptr);
        SwapChainImages.resize(ImageCount);
        vkGetSwapchainImagesKHR(Device, SwapChain, &ImageCount, SwapChainImages.data());
        SwapChainImageFormat = SurfaceFormat.format;
        SwapChainExtent = Extent;

    }

    void CleanUpSwapChain()
    {
        for (auto Framebuffer : SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(Device, Framebuffer, nullptr);
        }

        vkFreeCommandBuffers(Device, CommandPool, static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());

        vkDestroyPipeline(Device, GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        vkDestroyRenderPass(Device, RenderPass, nullptr);

        for (auto ImageView : SwapChainImageViews)
        {
            vkDestroyImageView(Device, ImageView, nullptr);
        }

        vkDestroySwapchainKHR(Device, SwapChain, nullptr);
    }

    void RecreateSwapChain()
    {
        int Width = 0;
        int Height = 0;
        glfwGetFramebufferSize(Window, &Width, &Height);
        while (Width == 0 || Height == 0)
        {
            glfwGetFramebufferSize(Window, &Width, &Height);
            glfwPollEvents();
        }
        
        vkDeviceWaitIdle(Device);

        CleanUpSwapChain();

        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandBuffers();
    }

    void SetupDebugMessenger()
    {
        if (!bEnableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT CreationInfo;
        PopulateDebugMessengerCreateInfo(CreationInfo);

        if (CreateDebugUtilsMessengerEXT(Instance, &CreationInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    bool IsDeviceSuitable(VkPhysicalDevice Device)
    {
        QueueFamilyIndices Indices = FindQueueFamilies(Device);

        bool ExtensionsSupported = CheckDeviceExtensionSupport(Device);

        bool SwapChainAdequate = false;

        if (ExtensionsSupported)
        {
            SwapChainSupportDetails SwapChainSupport = QuerrySwapChainSupport(Device);
            SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
        }

        return Indices.IsComplete() && ExtensionsSupported && SwapChainAdequate;
    }

    void PickPhysicalDevice()
    {
        uint DeviceCount = 0;
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

        if (DeviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> Devices(DeviceCount);
        vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

        for (const auto& Device : Devices)
        {
            if (IsDeviceSuitable(Device))
            {
                PhysicalDevice = Device;
                break;
            }
        }

        if (PhysicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice Device)
    {
        QueueFamilyIndices Indices;

        uint QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueFamilyCount, QueueFamilies.data());

        uint i = 0;
        for (const auto& Family : QueueFamilies)
        {
            VkBool32 PresentSupport = false;

            vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &PresentSupport);

            if (PresentSupport)
            {
                Indices.PresentFamily = i;
            }

            if (Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                Indices.GraphicsFamily = i;
            }

            if (Indices.IsComplete())
            {
                break;
            }

            ++i;
        }

        return Indices;
    }

    void CreateLogicalDevice()
    {
        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        std::set<uint> UniqueQueueFamilies = {Indices.GraphicsFamily.value(), Indices.PresentFamily.value()};

        float QueuePriority = 1.f;
        for (uint QueueFamily : UniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo QueueCreateInfo{};
            QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfo.queueFamilyIndex = QueueFamily;
            QueueCreateInfo.queueCount = 1;
            QueueCreateInfo.pQueuePriorities = &QueuePriority;
            QueueCreateInfos.push_back(QueueCreateInfo);
        }

        VkPhysicalDeviceFeatures DeviceFeatures{};

        VkDeviceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
        CreateInfo.queueCreateInfoCount = static_cast<uint>(QueueCreateInfos.size());
        CreateInfo.pEnabledFeatures = &DeviceFeatures;
        CreateInfo.enabledExtensionCount = static_cast<uint>(DeviceExtensions.size());
        CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
        if (bEnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = static_cast<uint>(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        else
        {
            CreateInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(Device, Indices.GraphicsFamily.value(), 0, &GraphicsQueue);
        vkGetDeviceQueue(Device, Indices.PresentFamily.value(), 0, &PresentQueue);
    }

    void CreateSurface()
    {
        if (glfwCreateWindowSurface(Instance, Window, nullptr, &Surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    void CreateImageViews()
    {
        SwapChainImageViews.resize(SwapChainImages.size());

        for (std::size_t i = 0; i < SwapChainImages.size(); ++i)
        {
            VkImageViewCreateInfo  CreateInfo{};
            CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            CreateInfo.image = SwapChainImages[i];
            CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            CreateInfo.format = SwapChainImageFormat;
            CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            CreateInfo.subresourceRange.baseMipLevel = 0;
            CreateInfo.subresourceRange.levelCount = 1;
            CreateInfo.subresourceRange.baseArrayLayer = 0;
            CreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(Device, &CreateInfo, nullptr, &SwapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    VkShaderModule CreateShaderModule(const std::vector<char>& Code)
    {
        VkShaderModuleCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        CreateInfo.codeSize = Code.size();
        CreateInfo.pCode = reinterpret_cast<const uint*>(Code.data());

        VkShaderModule ShaderModule;
        if (vkCreateShaderModule(Device, &CreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return ShaderModule;
    }

    void CreateGraphicsPipeline()
    {
        auto VertexShaderCode = ReadFile("../shaders/triangle_vert.spv");
        auto FragmentShaderCode = ReadFile("../shaders/triangle_frag.spv");

        VkShaderModule VertexShaderModule = CreateShaderModule(VertexShaderCode);
        VkShaderModule FragmentShaderModule = CreateShaderModule(FragmentShaderCode);

        VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = VertexShaderModule;
        VertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo FragmentShaderStageInfo{};
        FragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragmentShaderStageInfo.module = FragmentShaderModule;
        FragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo ShaderStages[] = {VertShaderStageInfo, FragmentShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputInfo.vertexBindingDescriptionCount = 0;
        VertexInputInfo.pVertexBindingDescriptions = nullptr;
        VertexInputInfo.vertexAttributeDescriptionCount = 0;
        VertexInputInfo.pVertexAttributeDescriptions = 0;

        VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
        InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport Viewport{};
        Viewport.x = 0.f;
        Viewport.y = 0.f;
        Viewport.width = (float)SwapChainExtent.width;
        Viewport.height = (float)SwapChainExtent.height;
        Viewport.minDepth = 0.f;
        Viewport.maxDepth = 1.f;

        VkRect2D Scissors{};
        Scissors.offset = {0, 0};
        Scissors.extent = SwapChainExtent;

        VkPipelineViewportStateCreateInfo ViewportState{};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 1;
        ViewportState.pViewports = &Viewport;
        ViewportState.scissorCount = 1;
        ViewportState.pScissors = &Scissors;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth = 1.f;
        Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        Rasterizer.depthBiasEnable = VK_FALSE;
        Rasterizer.depthBiasConstantFactor = 0.f;
        Rasterizer.depthBiasClamp = 0.f;
        Rasterizer.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        Multisampling.minSampleShading = 1.f;
        Multisampling.pSampleMask = nullptr;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_FALSE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo ColorBlending{};
        ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlending.logicOpEnable = VK_FALSE;
        ColorBlending.logicOp = VK_LOGIC_OP_COPY;
        ColorBlending.attachmentCount = 1;
        ColorBlending.pAttachments = &ColorBlendAttachment;
        ColorBlending.blendConstants[0] = 0.f;
        ColorBlending.blendConstants[1] = 0.f;
        ColorBlending.blendConstants[2] = 0.f;
        ColorBlending.blendConstants[3] = 0.f;


        VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 0;
        PipelineLayoutInfo.pSetLayouts = nullptr;
        PipelineLayoutInfo.pushConstantRangeCount = 0;
        PipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineInfo.stageCount = 2;
        PipelineInfo.pStages = ShaderStages;
        PipelineInfo.pVertexInputState = &VertexInputInfo;
        PipelineInfo.pInputAssemblyState = &InputAssembly;
        PipelineInfo.pViewportState = &ViewportState;
        PipelineInfo.pRasterizationState = &Rasterizer;
        PipelineInfo.pMultisampleState = &Multisampling;
        PipelineInfo.pDepthStencilState = nullptr;
        PipelineInfo.pColorBlendState = &ColorBlending;
        PipelineInfo.pDepthStencilState = nullptr;
        PipelineInfo.layout = PipelineLayout;
        PipelineInfo.renderPass = RenderPass;
        PipelineInfo.subpass = 0;
        PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(Device, FragmentShaderModule, nullptr);
        vkDestroyShaderModule(Device, VertexShaderModule, nullptr);
    }

    void CreateRenderPass()
    {
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = SwapChainImageFormat;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference ColorAttachmentRef{};
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDependency Dependency{};
        Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass = 0;
        Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.srcAccessMask = 0;
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDescription Subpass{};
        Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        Subpass.colorAttachmentCount = 1;
        Subpass.pColorAttachments = &ColorAttachmentRef;

        VkRenderPassCreateInfo RenderPassInfo{};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassInfo.attachmentCount = 1;
        RenderPassInfo.pAttachments = &ColorAttachment;
        RenderPassInfo.subpassCount = 1;
        RenderPassInfo.pSubpasses = &Subpass;
        RenderPassInfo.dependencyCount = 1;
        RenderPassInfo.pDependencies  = &Dependency;

        if (vkCreateRenderPass(Device, &RenderPassInfo, nullptr, &RenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void CreateFramebuffers()
    {
        SwapChainFramebuffers.resize(SwapChainImageViews.size());
        for (std::size_t i = 0; i < SwapChainImageViews.size(); ++i) {
            VkImageView Attachments[] = {SwapChainImageViews[i]};

            VkFramebufferCreateInfo FramebufferInfo{};
            FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FramebufferInfo.renderPass = RenderPass;
            FramebufferInfo.attachmentCount = 1;
            FramebufferInfo.pAttachments = Attachments;
            FramebufferInfo.width = SwapChainExtent.width;
            FramebufferInfo.height = SwapChainExtent.height;
            FramebufferInfo.layers = 1;

            if (vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void CreateCommandPool()
    {
        QueueFamilyIndices QueueFamilyIndices = FindQueueFamilies(PhysicalDevice);
        VkCommandPoolCreateInfo PoolInfo{};
        PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        PoolInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily.value();
        PoolInfo.flags = 0;

        if (vkCreateCommandPool(Device, &PoolInfo, nullptr, &CommandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void CreateCommandBuffers()
    {
        CommandBuffers.resize(SwapChainFramebuffers.size());

        VkCommandBufferAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        AllocInfo.commandPool = CommandPool;
        AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocInfo.commandBufferCount = (uint) CommandBuffers.size();

        if (vkAllocateCommandBuffers(Device, &AllocInfo, CommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (std::size_t i = 0; i <CommandBuffers.size(); ++i)
        {
            VkCommandBufferBeginInfo BeginInfo{};
            BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            BeginInfo.flags = 0;
            BeginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(CommandBuffers[i], &BeginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo RenderPassInfo{};
            RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            RenderPassInfo.renderPass = RenderPass;
            RenderPassInfo.framebuffer = SwapChainFramebuffers[i];
            RenderPassInfo.renderArea.offset = {0, 0};
            RenderPassInfo.renderArea.extent = SwapChainExtent;

            VkClearValue ClearColor = {0.f, 0.f, 0.f, 1.f};
            RenderPassInfo.clearValueCount = 1;
            RenderPassInfo.pClearValues = &ClearColor;

            vkCmdBeginRenderPass(CommandBuffers[i], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);
            vkCmdDraw(CommandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(CommandBuffers[i]);

            if (vkEndCommandBuffer(CommandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to record command buffer!");
            }
        }
    }

    void CreateSyncObjects()
    {
        ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        ImagesInFlight.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo SemaphoreInfo{};
        SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo FenceInfo{};
        FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(Device, &FenceInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }
    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(Window))
        {
            glfwPollEvents();
            DrawFrame();
        }
        vkDeviceWaitIdle(Device);
    }

    void DrawFrame()
    {
        vkWaitForFences(Device, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);
        uint ImageIndex;
        VkResult Result = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

        if (Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            return;
        }
        else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        if(ImagesInFlight[ImageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(Device, 1, &ImagesInFlight[ImageIndex], VK_TRUE, UINT64_MAX);
        }

        ImagesInFlight[ImageIndex] = InFlightFences[CurrentFrame];

        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore WaitSemaphores[] = {ImageAvailableSemaphores[CurrentFrame]};
        VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = WaitSemaphores;
        SubmitInfo.pWaitDstStageMask = WaitStages;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffers[ImageIndex];

        VkSemaphore SignalSemaphores[] = {RenderFinishedSemaphores[CurrentFrame]};
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = SignalSemaphores;

        vkResetFences(Device, 1, &InFlightFences[CurrentFrame]);

        if (vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFences[CurrentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR PresentInfo{};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = SignalSemaphores;
        VkSwapchainKHR SwapChains[] = {SwapChain};
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = SwapChains;
        PresentInfo.pImageIndices = &ImageIndex;
        PresentInfo.pResults = nullptr;

        Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);

        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || bFramebufferResized)
        {
            bFramebufferResized = false;
            RecreateSwapChain();
        }
        else if (Result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Cleanup()
    {
        CleanUpSwapChain();

        for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(Device, RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(Device, InFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(Device, CommandPool, nullptr);
        vkDestroyDevice(Device, nullptr);

        if (bEnableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(Instance, Surface, nullptr);
        vkDestroyInstance(Instance, nullptr);
        glfwDestroyWindow(Window);
        glfwTerminate();
    }

    GLFWwindow* Window;

    VkInstance Instance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    VkDevice Device;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkSurfaceKHR Surface;
    VkSwapchainKHR SwapChain;
    std::vector<VkImage> SwapChainImages;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    std::vector<VkImageView> SwapChainImageViews;
    VkPipelineLayout PipelineLayout;
    VkRenderPass RenderPass;
    VkPipeline GraphicsPipeline;
    std::vector<VkFramebuffer> SwapChainFramebuffers;
    VkCommandPool CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> InFlightFences;
    std::vector<VkFence> ImagesInFlight;
    size_t CurrentFrame = 0;
    bool bFramebufferResized = false;

};

int main()
{
    FHelloTriangleApplication App;

    try {
        App.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
