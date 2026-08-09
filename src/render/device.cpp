#include "device.hpp"

#include <iostream>
#include <limits>
#include <set>
#include <string>

#include "vulkan/vulkan_handles.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ||
        messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "\n##### " << pCallbackData->pMessage << std::endl;
    }
    // std::cerr << "\n##### " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

vk::Instance Device::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Misanthrope";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
    appInfo.pEngineName = "Misanthrope";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vk::createInstance(&createInfo, nullptr, &m_instance) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create instance!");
    }

    return m_instance;
}

void Device::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char*> Device::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Device::checkValidationLayerSupport() {
    uint32_t layerCount;
    vk::Result res = vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    res = vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Device::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT((VkInstance)&m_instance, &createInfo, nullptr,
                                     &m_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

bool Device::checkDeviceExtensionSupport(vk::PhysicalDevice i_device) {
    uint32_t extensionCount;
    i_device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
    i_device.enumerateDeviceExtensionProperties(nullptr, &extensionCount,
                                                availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        // std::cout << extension.extensionName << "\n";
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Device::querySwapChainSupport(vk::PhysicalDevice i_device) {
    SwapChainSupportDetails details;

    i_device.getSurfaceCapabilitiesKHR(m_surface, &details.capabilities);

    uint32_t formatCount;
    i_device.getSurfaceFormatsKHR(m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        i_device.getSurfaceFormatsKHR(m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    i_device.getSurfacePresentModesKHR(m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        i_device.getSurfacePresentModesKHR(m_surface, &presentModeCount,
                                           details.presentModes.data());
    }

    return details;
}

bool Device::isDeviceSuitable(vk::PhysicalDevice i_device) {
    QueueFamilyIndices indices = findQueueFamilies(i_device);

    bool extensionsSupported = checkDeviceExtensionSupport(i_device);

    vk::PhysicalDeviceProperties deviceProps{};
    i_device.getProperties(&deviceProps);
    std::cout << deviceProps.deviceName << std::endl;
    bool isIntegrateGPU =
        deviceProps.deviceType == vk::PhysicalDeviceType::eIntegratedGpu ? true : false;

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(i_device);
        swapChainAdequate =
            !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures;
    i_device.getFeatures(&supportedFeatures);

    return indices.isComplete() && isIntegrateGPU && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}

vk::SampleCountFlagBits Device::getMaxUsableSampleCount() {
    m_physicalDevice.getProperties(&m_physicalDeviceProperties);

    vk::SampleCountFlags counts = m_physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                  m_physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}

vk::Format Device::findSupportedFormat(const std::vector<vk::Format>& candidates,
                                       vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        m_physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format Device::findHDRColorFormat() {
    return findSupportedFormat({vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat,
                                vk::Format::eR8G8B8A8Srgb},
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
}

vk::Format Device::findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool Device::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void Device::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    m_instance.enumeratePhysicalDevices(&deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<vk::PhysicalDevice> devices(deviceCount);
    m_instance.enumeratePhysicalDevices(&deviceCount, devices.data());
    std::cout << "Physical device count: " << deviceCount << std::endl;

    for (unsigned int i = 0; i < devices.size(); i++) {
        if (isDeviceSuitable(devices[i])) {
            m_physicalDevice = devices[i];
            m_msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    m_physicalDevice.getProperties(&m_physicalDeviceProperties);
    m_renderTargetImageFormat = findHDRColorFormat();
    m_depthFormat = findDepthFormat();
    std::cout << "m_renderTargetImageFormat: "
              << vk::to_string(vk::Format(m_renderTargetImageFormat)) << "\n";

    std::vector<vk::PhysicalDeviceToolProperties> toolProps;
    toolProps = m_physicalDevice.getToolProperties();
    for (auto& toolProp : toolProps) {
        printf("%s:\n", &*toolProp.name);
        printf("Version:\n");
        printf("%s:\n", &*toolProp.version);
        printf("Description:\n");
        printf("\t%s\n", &*toolProp.description);
        printf("Purposes:\n");
        if (strnlen(toolProp.layer, VK_MAX_EXTENSION_NAME_SIZE) > 0) {
            printf("Corresponding Layer:\n");
            printf("\t%s\n", &*toolProp.layer);
        }
    }
}

// chosing desired sureface format
void Device::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicFamily.value(), indices.presentFamily.value()};

    if (indices.graphicFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (m_device.createSwapchainKHR(&createInfo, nullptr, &m_swapChain) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create swap chain!");
    }
    m_swapChainImages = m_device.getSwapchainImagesKHR(m_swapChain);

    m_swapchainImageFormat = surfaceFormat.format;
    std::cout << "Swapchain format: " << vk::to_string(m_swapchainImageFormat) << "\n";
    std::cout << "Swapchain images count: " << imageCount << "\n";
    m_swapChainExtent = extent;
    m_shadowExtent = m_swapChainExtent;
    m_swapchainProperties = swapChainSupport;
}

void Device::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicFamily.value(),
                                              indices.computeFamily.value(),
                                              indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    vk::PhysicalDeviceVertexAttributeDivisorFeaturesEXT divisorFeature{};
    divisorFeature.vertexAttributeInstanceRateDivisor = true;
    divisorFeature.vertexAttributeInstanceRateZeroDivisor = true;

    vk::PhysicalDeviceRobustness2FeaturesEXT robustFeature{};
    robustFeature.nullDescriptor = true;
    robustFeature.robustBufferAccess2 = false;
    robustFeature.robustImageAccess2 = false;
    robustFeature.pNext = &divisorFeature;

    // vk::PhysicalDeviceRobustness2PropertiesEXT robustProperties{};
    // robustProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;
    // robustProperties.robustUniformBufferAccessSizeAlignment = 256;

    // robustFeature.pNext = &robustProperties;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicState{};
    extendedDynamicState.extendedDynamicState = 1;
    extendedDynamicState.pNext = nullptr;
    extendedDynamicState.pNext = &robustFeature;

    vk::PhysicalDeviceVulkan12Features enabledVk12Features{};
    enabledVk12Features.descriptorIndexing = true;
    enabledVk12Features.shaderSampledImageArrayNonUniformIndexing = true;
    enabledVk12Features.descriptorBindingVariableDescriptorCount = true;
    enabledVk12Features.runtimeDescriptorArray = true;
    enabledVk12Features.bufferDeviceAddress = true;
    enabledVk12Features.pNext = &extendedDynamicState;

    VkPhysicalDeviceVulkan13Features enabledVk13Features{};
    enabledVk13Features.synchronization2 = true;
    enabledVk13Features.dynamicRendering = true;
    enabledVk13Features.pNext = &enabledVk12Features;

    createInfo.pNext = &enabledVk13Features;

    if (m_physicalDevice.createDevice(&createInfo, nullptr, &m_device) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create logical device!");
    }

    m_device.getQueue(indices.graphicFamily.value(), 0, &m_graphicQueue);
    m_device.getQueue(indices.graphicFamily.value(), 0, &m_computeQueue);
    m_device.getQueue(indices.graphicFamily.value(), 0, &m_presentQueue);

    // std::cout << "\nQueue graphic family Index: " << indices.graphicFamily.value()
    // 		<< "\nQueue compute family Index: " << indices.computeFamily.value()
    // 		<< "\nQueue present family Index: " << indices.presentFamily.value() << std::endl;

    m_vkCmdSetPrimitiveTopologyEXT =
        (PFN_vkCmdSetPrimitiveTopologyEXT)m_device.getProcAddr("vkCmdSetPrimitiveTopologyEXT");
}

QueueFamilyIndices Device::findQueueFamilies(vk::PhysicalDevice i_device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    i_device.getQueueFamilyProperties(&queueFamilyCount, nullptr);

    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    i_device.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicFamily = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) indices.computeFamily = i;

        vk::Bool32 presentSupport = false;
        i_device.getSurfaceSupportKHR(i, m_surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

vk::SurfaceFormatKHR Device::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Device::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    std::cout << "Available present mode: " << "\n";
    for (const auto& availablePresentMode : availablePresentModes) {
        std::cout << vk::to_string((vk::PresentModeKHR)availablePresentMode) << "\n";
        if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eImmediate;
}

vk::Extent2D Device::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

vk::Bool32 Device::isFormatFilterable(vk::Format format, vk::ImageTiling tiling) {
    vk::FormatProperties formatProps;
    m_physicalDevice.getFormatProperties(format, &formatProps);

    if (tiling == vk::ImageTiling::eOptimal)
        return bool(formatProps.optimalTilingFeatures &
                    vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

    if (tiling == vk::ImageTiling::eLinear)
        return bool(formatProps.linearTilingFeatures &
                    vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

    return false;
}

void Device::printPhysicalDeviceProperties() {
    std::cout
        << "####### Physical device info: #######" << "\n apiVersion: \n"
        << m_physicalDeviceProperties.apiVersion << "\n driverVersion: \n"
        << m_physicalDeviceProperties.driverVersion << "\n vendorID: \n"
        << m_physicalDeviceProperties.vendorID << "\n deviceID: \n"
        << m_physicalDeviceProperties.deviceID << "\n deviceType: \n"
        << vk::to_string(m_physicalDeviceProperties.deviceType) <<
        // "\n deviceName: \n" << m_physicalDeviceProperties.deviceID <<
        "\n ####### Physical device properties: #######\n"
        << "\n maxImageDimension1D: " << m_physicalDeviceProperties.limits.maxImageDimension1D
        << "\n maxImageDimension2D: " << m_physicalDeviceProperties.limits.maxImageDimension2D
        << "\n maxImageDimension3D: " << m_physicalDeviceProperties.limits.maxImageDimension3D
        << "\n maxImageDimensionCube: " << m_physicalDeviceProperties.limits.maxImageDimensionCube
        << "\n maxImageArrayLayers: " << m_physicalDeviceProperties.limits.maxImageArrayLayers
        << "\n maxTexelBufferElements: " << m_physicalDeviceProperties.limits.maxTexelBufferElements
        << "\n maxUniformBufferRange: " << m_physicalDeviceProperties.limits.maxUniformBufferRange
        << "\n maxStorageBufferRange: " << m_physicalDeviceProperties.limits.maxStorageBufferRange
        << "\n maxPushConstantsSize: " << m_physicalDeviceProperties.limits.maxPushConstantsSize
        << "\n maxMemoryAllocationCount: "
        << m_physicalDeviceProperties.limits.maxMemoryAllocationCount
        << "\n maxSamplerAllocationCount: "
        << m_physicalDeviceProperties.limits.maxSamplerAllocationCount
        << "\n bufferImageGranularity: " << m_physicalDeviceProperties.limits.bufferImageGranularity
        << "\n sparseAddressSpaceSize: " << m_physicalDeviceProperties.limits.sparseAddressSpaceSize
        << "\n maxBoundDescriptorSets: " << m_physicalDeviceProperties.limits.maxBoundDescriptorSets
        << "\n maxPerStageDescriptorSamplers: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorSamplers
        << "\n maxPerStageDescriptorUniformBuffers: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers
        << "\n maxPerStageDescriptorStorageBuffers: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers
        << "\n maxPerStageDescriptorSampledImages: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages
        << "\n maxPerStageDescriptorStorageImages: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages
        << "\n maxPerStageDescriptorInputAttachments: "
        << m_physicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments
        << "\n maxPerStageResources: " << m_physicalDeviceProperties.limits.maxPerStageResources
        << "\n maxDescriptorSetSamplers: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetSamplers
        << "\n maxDescriptorSetUniformBuffers: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers
        << "\n maxDescriptorSetUniformBuffersDynamic: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic
        << "\n maxDescriptorSetStorageBuffers: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers
        << "\n maxDescriptorSetStorageBuffersDynamic: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic
        << "\n maxDescriptorSetSampledImages: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetSampledImages
        << "\n maxDescriptorSetStorageImages: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetStorageImages
        << "\n maxDescriptorSetInputAttachments: "
        << m_physicalDeviceProperties.limits.maxDescriptorSetInputAttachments
        << "\n maxVertexInputAttributes: "
        << m_physicalDeviceProperties.limits.maxVertexInputAttributes
        << "\n maxVertexInputBindings: " << m_physicalDeviceProperties.limits.maxVertexInputBindings
        << "\n maxVertexInputAttributeOffset: "
        << m_physicalDeviceProperties.limits.maxVertexInputAttributeOffset
        << "\n maxVertexInputBindingStride: "
        << m_physicalDeviceProperties.limits.maxVertexInputBindingStride
        << "\n maxVertexOutputComponents: "
        << m_physicalDeviceProperties.limits.maxVertexOutputComponents
        << "\n maxTessellationGenerationLevel: "
        << m_physicalDeviceProperties.limits.maxTessellationGenerationLevel
        << "\n maxTessellationPatchSize: "
        << m_physicalDeviceProperties.limits.maxTessellationPatchSize
        << "\n maxTessellationControlPerVertexInputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents
        << "\n maxTessellationControlPerVertexOutputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents
        << "\n maxTessellationControlPerPatchOutputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents
        << "\n maxTessellationControlTotalOutputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents
        << "\n maxTessellationEvaluationInputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationEvaluationInputComponents
        << "\n maxTessellationEvaluationOutputComponents: "
        << m_physicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents
        << "\n maxGeometryShaderInvocations: "
        << m_physicalDeviceProperties.limits.maxGeometryShaderInvocations
        << "\n maxGeometryInputComponents: "
        << m_physicalDeviceProperties.limits.maxGeometryInputComponents
        << "\n maxGeometryOutputComponents: "
        << m_physicalDeviceProperties.limits.maxGeometryOutputComponents
        << "\n maxGeometryOutputVertices: "
        << m_physicalDeviceProperties.limits.maxGeometryOutputVertices
        << "\n maxGeometryTotalOutputComponents: "
        << m_physicalDeviceProperties.limits.maxGeometryTotalOutputComponents
        << "\n maxFragmentInputComponents: "
        << m_physicalDeviceProperties.limits.maxFragmentInputComponents
        << "\n maxFragmentOutputAttachments: "
        << m_physicalDeviceProperties.limits.maxFragmentOutputAttachments
        << "\n maxFragmentDualSrcAttachments: "
        << m_physicalDeviceProperties.limits.maxFragmentDualSrcAttachments
        << "\n maxFragmentCombinedOutputResources: "
        << m_physicalDeviceProperties.limits.maxFragmentCombinedOutputResources
        << "\n maxComputeSharedMemorySize: "
        << m_physicalDeviceProperties.limits.maxComputeSharedMemorySize
        << "\n maxComputeWorkGroupCount[0]: "
        << m_physicalDeviceProperties.limits.maxComputeWorkGroupCount[0]
        << "\n maxComputeWorkGroupInvocations: "
        << m_physicalDeviceProperties.limits.maxComputeWorkGroupInvocations
        << "\n maxComputeWorkGroupSize[0]: "
        << m_physicalDeviceProperties.limits.maxComputeWorkGroupSize[0]
        << "\n subPixelPrecisionBits: " << m_physicalDeviceProperties.limits.subPixelPrecisionBits
        << "\n subTexelPrecisionBits: " << m_physicalDeviceProperties.limits.subTexelPrecisionBits
        << "\n mipmapPrecisionBits: " << m_physicalDeviceProperties.limits.mipmapPrecisionBits
        << "\n maxDrawIndexedIndexValue: "
        << m_physicalDeviceProperties.limits.maxDrawIndexedIndexValue
        << "\n maxDrawIndirectCount: " << m_physicalDeviceProperties.limits.maxDrawIndirectCount
        << "\n maxSamplerLodBias: " << m_physicalDeviceProperties.limits.maxSamplerLodBias
        << "\n maxSamplerAnisotropy: " << m_physicalDeviceProperties.limits.maxSamplerAnisotropy
        << "\n maxViewports: " << m_physicalDeviceProperties.limits.maxViewports
        << "\n maxViewportDimensions[0]: "
        << m_physicalDeviceProperties.limits.maxViewportDimensions[0]
        << "\n viewportBoundsRange[0]: " << m_physicalDeviceProperties.limits.viewportBoundsRange[0]
        << "\n viewportSubPixelBits: " << m_physicalDeviceProperties.limits.viewportSubPixelBits
        << "\n minMemoryMapAlignment: " << m_physicalDeviceProperties.limits.minMemoryMapAlignment
        << "\n minTexelBufferOffsetAlignment: "
        << m_physicalDeviceProperties.limits.minTexelBufferOffsetAlignment
        << "\n minUniformBufferOffsetAlignment: "
        << m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment
        << "\n minStorageBufferOffsetAlignment: "
        << m_physicalDeviceProperties.limits.minStorageBufferOffsetAlignment
        << "\n minTexelOffset: " << m_physicalDeviceProperties.limits.minTexelOffset
        << "\n maxTexelOffset: " << m_physicalDeviceProperties.limits.maxTexelOffset
        << "\n minTexelGatherOffset: " << m_physicalDeviceProperties.limits.minTexelGatherOffset
        << "\n maxTexelGatherOffset: " << m_physicalDeviceProperties.limits.maxTexelGatherOffset
        << "\n minInterpolationOffset: " << m_physicalDeviceProperties.limits.minInterpolationOffset
        << "\n maxInterpolationOffset: " << m_physicalDeviceProperties.limits.maxInterpolationOffset
        << "\n subPixelInterpolationOffsetBits: "
        << m_physicalDeviceProperties.limits.subPixelInterpolationOffsetBits
        << "\n maxFramebufferWidth: " << m_physicalDeviceProperties.limits.maxFramebufferWidth
        << "\n maxFramebufferHeight: " << m_physicalDeviceProperties.limits.maxFramebufferHeight
        << "\n maxFramebufferLayers: " << m_physicalDeviceProperties.limits.maxFramebufferLayers
        << "\n framebufferColorSampleCounts: "
        << vk::to_string(m_physicalDeviceProperties.limits.framebufferColorSampleCounts)
        << "\n maxSampleMaskWords: " << m_physicalDeviceProperties.limits.maxSampleMaskWords
        << "\n timestampComputeAndGraphics: "
        << m_physicalDeviceProperties.limits.timestampComputeAndGraphics
        << "\n timestampPeriod: " << m_physicalDeviceProperties.limits.timestampPeriod
        << "\n maxClipDistances: " << m_physicalDeviceProperties.limits.maxClipDistances
        << "\n maxCullDistances: " << m_physicalDeviceProperties.limits.maxCullDistances
        << "\n maxCombinedClipAndCullDistances: "
        << m_physicalDeviceProperties.limits.maxCombinedClipAndCullDistances
        << "\n discreteQueuePriorities: "
        << m_physicalDeviceProperties.limits.discreteQueuePriorities
        << "\n pointSizeGranularity: " << m_physicalDeviceProperties.limits.pointSizeGranularity
        << "\n lineWidthGranularity: " << m_physicalDeviceProperties.limits.lineWidthGranularity
        << "\n strictLines: " << m_physicalDeviceProperties.limits.strictLines
        << "\n standardSampleLocations: "
        << m_physicalDeviceProperties.limits.standardSampleLocations
        << "\n optimalBufferCopyOffsetAlignment: "
        << m_physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment
        << "\n optimalBufferCopyRowPitchAlignment: "
        << m_physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment
        << "\n nonCoherentAtomSize: " << m_physicalDeviceProperties.limits.nonCoherentAtomSize
        << "\n";
}

void Device::printSwapchainProperties() {
    std::cout << "\n ####### Swapchain Properties: #######"
              << "\n Min images count: " << m_swapchainProperties.capabilities.minImageCount
              << "\n Max images count: " << m_swapchainProperties.capabilities.maxImageCount
              << "\n Current images extent width: "
              << m_swapchainProperties.capabilities.currentExtent.width
              << "\n Current images extent height: "
              << m_swapchainProperties.capabilities.currentExtent.height
              << "\n Min images extent width: "
              << m_swapchainProperties.capabilities.minImageExtent.width
              << "\n Min images extent height: "
              << m_swapchainProperties.capabilities.minImageExtent.height
              << "\n Max images extent width: "
              << m_swapchainProperties.capabilities.maxImageExtent.width
              << "\n Max images extent height: "
              << m_swapchainProperties.capabilities.maxImageExtent.height
              << "\n Max Image Array Layers: "
              << m_swapchainProperties.capabilities.maxImageArrayLayers;

    std::cout << "\n ####### Supported Swapchain Formats: #######\n";
    for (auto& format : m_swapchainProperties.formats) {
        std::cout << vk::to_string((vk::Format)format.format) << "\n";
    }

    std::cout << "\n####### Supported Swapchain Present Mode: #######\n";
    for (auto& presentMode : m_swapchainProperties.presentModes) {
        std::cout << vk::to_string((vk::PresentModeKHR)presentMode) << "\n";
    }
}

void Device::printPhysicalDeviceFeatures() {
    // vkGetPhysicalDeviceFeatures();
}

void Device::printPhysicalDeviceFormats() {
    // for (vk::Format format : candidates) {
    //     vk::FormatProperties props;
    //     vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
    // }
}

void Device::printQueueFamilyProperties() {
    uint32_t count{};
    std::vector<vk::QueueFamilyProperties> queueProperties =
        m_physicalDevice.getQueueFamilyProperties();

    std::cout << "\n####### Queue Family index: #######" << "\n";
    for (unsigned int i = 0; i < queueProperties.size(); i++) {
        std::cout << "At index: " << i << ": "
                  << vk::to_string((vk::QueueFlags)queueProperties[i].queueFlags) << "\n";
    }
}

vk::Instance Device::getInstance() {
    return m_instance;
}
vk::SurfaceKHR* Device::getSurfacePtr() {
    return &m_surface;
}

vk::Device Device::getVkDevice() {
    return m_device;
}

vk::Extent2D Device::getSwapchainExtent() {
    return m_swapChainExtent;
}

void Device::createDescriptorSetLayouts() {
    // snowflake
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        CHECK_VK_RESULT(m_device.createDescriptorSetLayout(
                            &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.snowflake),
                        "fail to create snowflake descriptor set layout");
    }

    // candles
    {
        // for candles: 2 descriptor set layouts, 1 for texture+sampler(change for each mesh), 1 for
        // uniform buffer (change each frame)
        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
            uboLayoutBinding.pImmutableSamplers = nullptr;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

            vk::DescriptorSetLayoutBinding lightBinding{};
            lightBinding.binding = 1;
            lightBinding.descriptorCount = 1;
            lightBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            lightBinding.pImmutableSamplers = nullptr;
            lightBinding.stageFlags =
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

            std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, lightBinding};
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(
                    device, &layoutInfo, nullptr,
                    &m_graphicDescriptorSetLayouts.candles.tranformUniform) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
        }

        {
            VkDescriptorSetLayoutBinding samplerLayoutBinding{};
            samplerLayoutBinding.binding = 2;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding normalBinding{};
            normalBinding.binding = 3;
            normalBinding.descriptorCount = 1;
            normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normalBinding.pImmutableSamplers = nullptr;
            normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding emissiveBinding{};
            emissiveBinding.binding = 4;
            emissiveBinding.descriptorCount = 1;
            emissiveBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            emissiveBinding.pImmutableSamplers = nullptr;
            emissiveBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            std::array<VkDescriptorSetLayoutBinding, 3> bindings = {samplerLayoutBinding,
                                                                    normalBinding, emissiveBinding};
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                            &m_graphicDescriptorSetLayouts.candles.meshMaterial) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
        }
    }

    // floor
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                                samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                    &m_graphicDescriptorSetLayouts.floor),
                        "fail to create snowflake descriptor set layout");
    }

    // skybox
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                                samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                    &m_graphicDescriptorSetLayouts.skybox),
                        "fail to create snowflake descriptor set layout");
    }

    // shadow
    {
        VkDescriptorSetLayoutBinding transform;
        transform.binding = 0;
        transform.descriptorCount = 1;
        transform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        transform.pImmutableSamplers = nullptr;
        transform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding perMeshTransform;
        perMeshTransform.binding = 1;
        perMeshTransform.descriptorCount = 1;
        perMeshTransform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        perMeshTransform.pImmutableSamplers = nullptr;
        perMeshTransform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding perInstanceTransform;
        perInstanceTransform.binding = 2;
        perInstanceTransform.descriptorCount = 1;
        perInstanceTransform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        perInstanceTransform.pImmutableSamplers = nullptr;
        perInstanceTransform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding pBinding[3] = {transform, perMeshTransform,
                                                    perInstanceTransform};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = pBinding;

        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                    &m_graphicDescriptorSetLayouts.shadow),
                        "fail to create shadow descriptor set layout");
    }

    // for bloom
    {
        VkDescriptorSetLayoutBinding baseBinding{};
        baseBinding.binding = 0;
        baseBinding.descriptorCount = 1;
        baseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        baseBinding.pImmutableSamplers = nullptr;
        baseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &baseBinding;

        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                    &m_graphicDescriptorSetLayouts.bloom),
                        "fail to create bloom descriptor set layout");
    }

    // for combine
    {
        VkDescriptorSetLayoutBinding baseBinding{};
        baseBinding.binding = 0;
        baseBinding.descriptorCount = 1;
        baseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        baseBinding.pImmutableSamplers = nullptr;
        baseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bloomBinding{};
        bloomBinding.binding = 1;
        bloomBinding.descriptorCount = 1;
        bloomBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bloomBinding.pImmutableSamplers = nullptr;
        bloomBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings{baseBinding, bloomBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                                    &m_graphicDescriptorSetLayouts.combine),
                        "fail to create bloom descriptor set layout");
    }

    // snowflake compute
    {
        VkDescriptorSetLayoutBinding inputStorageBinding{};
        inputStorageBinding.binding = 0;
        inputStorageBinding.descriptorCount = 1;
        inputStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        inputStorageBinding.pImmutableSamplers = nullptr;
        inputStorageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding outputStorageBinding{};
        outputStorageBinding.binding = 1;
        outputStorageBinding.descriptorCount = 1;
        outputStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        outputStorageBinding.pImmutableSamplers = nullptr;
        outputStorageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 2;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.pImmutableSamplers = nullptr;
        uboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = {inputStorageBinding,
                                                                outputStorageBinding, uboBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                        &m_computeDescriptorSetLayouts.snowflake) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
}

void Device::createPipelineCache() {
    if (!IsFileExist("../../res/cache/pipeline_cache.blob"))
        MakeFile("../../res/cache/pipeline_cache.blob");
    pipelineCacheBlob = ReadFile("../../res/cache/pipeline_cache.blob");

    VkPipelineCacheCreateInfo pipelineCacheInfo{};
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheInfo.pInitialData = static_cast<void*>(pipelineCacheBlob.data());
    pipelineCacheInfo.initialDataSize = pipelineCacheBlob.size() * sizeof(char);
    vkCreatePipelineCache(device, &pipelineCacheInfo, nullptr, &m_pipelineCache);
}
