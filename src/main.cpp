#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>

using uint = std::uint32_t;

const uint WIDTH = 1920;
const uint HEIGHT = 1080;

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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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

    void CreateImageView()
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

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageView();
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(Window))
        {
            glfwPollEvents();
        }
    }

    void Cleanup()
    {
        for (auto ImageView : SwapChainImageViews)
        {
            vkDestroyImageView(Device, ImageView, nullptr);
        }
        vkDestroySwapchainKHR(Device, SwapChain, nullptr);
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