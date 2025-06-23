#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"
#include "GLFW/include/glfw3.h"
#include "optional"
#include "memory"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
	VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
	VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME	
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
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device {
public:
	void createInstance();
	void createSurface(GLFWwindow* window);
	void createSwapChain();
	void createLogicalDevice();
	void setupDebugMessenger();

private:
bool checkValidationLayerSupport();
std::vector<const char*> getRequiredExtensions();
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
void pickPhysicalDevice();
bool isDeviceSuitable(VkPhysicalDevice device);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
VkSampleCountFlagBits getMaxUsableSampleCount();
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findHDRColorFormat();
VkFormat findDepthFormat();
bool hasStencilComponent(VkFormat format);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
VkBool32 isFormatFilterable(VkFormat format, VkImageTiling tiling);

void printPhysicalDeviceProperties();
void printSwapchainProperties();
void printPhysicalDeviceFeatures();
void printPhysicalDeviceFormats();
void printQueueFamilyProperties();

private:
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapChain;
    VkPhysicalDevice m_physicalDevice;
    VkQueue m_graphicQueue;
    VkQueue m_computeQueue;
    VkQueue m_presentQueue;
	std::shared_ptr<VkDevice> m_device;

    GLFWwindow* m_window;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPhysicalDeviceProperties m_physicalDeviceProperties;
	SwapChainSupportDetails m_swapchainProperties;

    std::vector<VkImage> m_swapChainImages;

    VkFormat m_swapchainImageFormat;
    VkExtent2D swapChainExtent;
    VkExtent2D m_shadowExtent;
	bool m_isHDR{false};
    VkFormat m_renderTargetImageFormat;
	VkFormat m_depthFormat;
	PFN_vkCmdSetPrimitiveTopologyEXT m_vkCmdSetPrimitiveTopologyEXT;
};
