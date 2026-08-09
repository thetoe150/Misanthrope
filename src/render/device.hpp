#ifndef DEVICE_H
#define DEVICE_H

#include "GLFW/include/glfw3.h"
#include "memory"
#include "optional"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define CHECK_VK_RESULT(f, msg)                                             \
    {                                                                       \
        if (VkResult res = f) {                                             \
            throw std::runtime_error(msg + vk::to_string((vk::Result)res)); \
        }                                                                   \
    }

const std::array<const char*, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::array<const char*, 4> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
    VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME
    // VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class Device {
   public:
    vk::Instance createInstance();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createSwapChain();
    void createLogicalDevice();
    void setupDebugMessenger();
    vk::Device getVkDevice();
    vk::Instance getInstance();
    vk::SurfaceKHR* getSurfacePtr();
    vk::Extent2D getSwapchainExtent();
    vk::ShaderModule createShaderModule(const std::vector<uint8_t>& code);
    VkImageView createImageView(vk::Image image, vk::Format format,
                                vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool isDeviceSuitable(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
    vk::SampleCountFlagBits getMaxUsableSampleCount();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findHDRColorFormat();
    vk::Format findDepthFormat();
    bool hasStencilComponent(vk::Format format);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    vk::Bool32 isFormatFilterable(vk::Format format, vk::ImageTiling tiling);

    void printPhysicalDeviceProperties();
    void printSwapchainProperties();
    void printPhysicalDeviceFeatures();
    void printPhysicalDeviceFormats();
    void printQueueFamilyProperties();

    vk::Instance m_instance;
    vk::SurfaceKHR m_surface;
    vk::SwapchainKHR m_swapChain;
    vk::PhysicalDevice m_physicalDevice;
    vk::Queue m_graphicQueue;
    vk::Queue m_computeQueue;
    vk::Queue m_presentQueue;
    vk::Device m_device;
    VmaAllocator m_allocator;

    GLFWwindow* m_window;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    vk::PhysicalDeviceProperties m_physicalDeviceProperties;
    SwapChainSupportDetails m_swapchainProperties;

    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapchainImageFormat;
    vk::Extent2D m_swapChainExtent;
    vk::Extent2D m_shadowExtent;
    bool m_isHDR{false};
    vk::Format m_renderTargetImageFormat;
    vk::Format m_depthFormat;
    PFN_vkCmdSetPrimitiveTopologyEXT m_vkCmdSetPrimitiveTopologyEXT;
};

#endif /* DEVICE_H */
