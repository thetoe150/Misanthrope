#include "device.hpp"
#include <iostream>
#include <string>
#include <set>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT || messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << "\n##### " << pCallbackData->pMessage << std::endl;
	}
	// std::cerr << "\n##### " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void Device::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Misanthrope";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
	appInfo.pEngineName = "Misanthrope";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Device::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Device::setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Device::createSurface(GLFWwindow* window) {
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		// std::cout << extension.extensionName << "\n";
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


bool Device::isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	VkPhysicalDeviceProperties deviceProps{};
	vkGetPhysicalDeviceProperties(device, &deviceProps);
	std::cout << deviceProps.deviceName << std::endl;
	bool isIntegrateGPU = deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? true : false;

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && isIntegrateGPU && extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount() {
	VkPhysicalDeviceProperties m_physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);

	VkSampleCountFlags counts = m_physicalDeviceProperties.limits.framebufferColorSampleCounts & m_physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat Device::findHDRColorFormat() {
	return findSupportedFormat(
		{VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_SRGB},
		VK_IMAGE_TILING_OPTIMAL,
	   VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT 
	);
}

VkFormat Device::findDepthFormat() {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Device::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Device::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
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

	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
	m_renderTargetImageFormat = findHDRColorFormat();
	m_depthFormat = findDepthFormat();
	std::cout << "m_renderTargetImageFormat: " << vk::to_string(vk::Format(m_renderTargetImageFormat)) << "\n";

	VkPhysicalDeviceToolProperties *toolProps;
	uint32_t toolNum;
	vkGetPhysicalDeviceToolProperties(m_physicalDevice, &toolNum, nullptr);
	toolProps = (VkPhysicalDeviceToolProperties*)malloc(sizeof(VkPhysicalDeviceToolProperties) * toolNum);
	vkGetPhysicalDeviceToolProperties(m_physicalDevice, &toolNum, toolProps);
	for (unsigned int i = 0; i < toolNum; i++) {
		printf("%s:\n", toolProps[i].name);
		printf("Version:\n");
		printf("%s:\n", toolProps[i].version);
		printf("Description:\n");
		printf("\t%s\n", toolProps[i].description);
		printf("Purposes:\n");
		if (strnlen(toolProps[i].layer, VK_MAX_EXTENSION_NAME_SIZE) > 0) {
			printf("Corresponding Layer:\n");
			printf("\t%s\n", toolProps[i].layer);
		}
	}
}

void Device::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicFamily.value(), indices.presentFamily.value()};

	if (indices.graphicFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(*m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(*m_device, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(*m_device, m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapchainImageFormat = surfaceFormat.format;
	std::cout << "Swapchain format: " << m_swapchainImageFormat << "\n";
	std::cout << "Swapchain images count: " << imageCount << "\n";
	swapChainExtent = extent;
	m_shadowExtent = {swapChainExtent.width * 2, swapChainExtent.height * 2};
	m_swapchainProperties = swapChainSupport; 
}

void Device::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicFamily.value(), indices.computeFamily.value(), indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

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

	VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT divisorFeature{};
	divisorFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
	divisorFeature.vertexAttributeInstanceRateDivisor = true;
	divisorFeature.vertexAttributeInstanceRateZeroDivisor = true;

	createInfo.pNext = &divisorFeature;

	VkPhysicalDeviceRobustness2FeaturesEXT robustFeature{};
	robustFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
	robustFeature.nullDescriptor = true;
	robustFeature.robustBufferAccess2 = false;
	robustFeature.robustImageAccess2 = false;

	divisorFeature.pNext = &robustFeature;

	// VkPhysicalDeviceRobustness2PropertiesEXT robustProperties{};
	// robustProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;
	// robustProperties.robustUniformBufferAccessSizeAlignment = 256;

	// robustFeature.pNext = &robustProperties;
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicState{};
	extendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	extendedDynamicState.extendedDynamicState = 1;
	extendedDynamicState.pNext = nullptr;

	robustFeature.pNext = &extendedDynamicState;

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &*m_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(*m_device, indices.graphicFamily.value(), 0, &m_graphicQueue);
	vkGetDeviceQueue(*m_device, indices.computeFamily.value(), 0, &m_computeQueue);
	vkGetDeviceQueue(*m_device, indices.presentFamily.value(), 0, &m_presentQueue);

	// std::cout << "\nQueue graphic family Index: " << indices.graphicFamily.value()
	// 		<< "\nQueue compute family Index: " << indices.computeFamily.value()
	// 		<< "\nQueue present family Index: " << indices.presentFamily.value() << std::endl;


	m_vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)vkGetDeviceProcAddr(*m_device, "vkCmdSetPrimitiveTopologyEXT");
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicFamily = i;
		}

		if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

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

VkSurfaceFormatKHR Device::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Device::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	std::cout << "Available present mode: " << "\n";
	for (const auto& availablePresentMode : availablePresentModes) {
		std::cout << vk::to_string((vk::PresentModeKHR)availablePresentMode) << "\n";
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D Device::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

VkBool32 Device::isFormatFilterable(VkFormat format, VkImageTiling tiling)
{
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProps);

	if (tiling == VK_IMAGE_TILING_OPTIMAL)
		return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	if (tiling == VK_IMAGE_TILING_LINEAR)
		return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	return false;
}


void Device::printPhysicalDeviceProperties(){
	std::cout << "####### Physical device info: #######" <<
	"\n apiVersion: \n" << m_physicalDeviceProperties.apiVersion <<
	"\n driverVersion: \n" << m_physicalDeviceProperties.driverVersion <<
	"\n vendorID: \n" << m_physicalDeviceProperties.vendorID <<
	"\n deviceID: \n" << m_physicalDeviceProperties.deviceID <<
	"\n deviceType: \n" << m_physicalDeviceProperties.deviceType <<
	// "\n deviceName: \n" << m_physicalDeviceProperties.deviceID <<
	"\n ####### Physical device properties: #######\n" <<
	"\n maxImageDimension1D: " << m_physicalDeviceProperties.limits.maxImageDimension1D <<
	"\n maxImageDimension2D: "     << m_physicalDeviceProperties.limits.maxImageDimension2D <<
	"\n maxImageDimension3D: "     << m_physicalDeviceProperties.limits.maxImageDimension3D <<
	"\n maxImageDimensionCube: "   << m_physicalDeviceProperties.limits.maxImageDimensionCube <<
	"\n maxImageArrayLayers: " << m_physicalDeviceProperties.limits.maxImageArrayLayers <<
	"\n maxTexelBufferElements: "  << m_physicalDeviceProperties.limits.maxTexelBufferElements <<
	"\n maxUniformBufferRange: "   << m_physicalDeviceProperties.limits.maxUniformBufferRange <<
	"\n maxStorageBufferRange: "   << m_physicalDeviceProperties.limits.maxStorageBufferRange <<
	"\n maxPushConstantsSize: "    << m_physicalDeviceProperties.limits.maxPushConstantsSize <<
	"\n maxMemoryAllocationCount: "    << m_physicalDeviceProperties.limits.maxMemoryAllocationCount <<
	"\n maxSamplerAllocationCount: "   << m_physicalDeviceProperties.limits.maxSamplerAllocationCount <<
	"\n bufferImageGranularity: "<< m_physicalDeviceProperties.limits.bufferImageGranularity <<
	"\n sparseAddressSpaceSize: "<< m_physicalDeviceProperties.limits.sparseAddressSpaceSize <<
	"\n maxBoundDescriptorSets: "  << m_physicalDeviceProperties.limits.maxBoundDescriptorSets <<
	"\n maxPerStageDescriptorSamplers: "   << m_physicalDeviceProperties.limits.maxPerStageDescriptorSamplers <<
	"\n maxPerStageDescriptorUniformBuffers: "     << m_physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers <<
	"\n maxPerStageDescriptorStorageBuffers: "     << m_physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers <<
	"\n maxPerStageDescriptorSampledImages: "  << m_physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages <<
	"\n maxPerStageDescriptorStorageImages: "  << m_physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages <<
	"\n maxPerStageDescriptorInputAttachments: "   << m_physicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments <<
	"\n maxPerStageResources: "    << m_physicalDeviceProperties.limits.maxPerStageResources <<
	"\n maxDescriptorSetSamplers: "    << m_physicalDeviceProperties.limits.maxDescriptorSetSamplers <<
	"\n maxDescriptorSetUniformBuffers: "  << m_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers <<
	"\n maxDescriptorSetUniformBuffersDynamic: "   << m_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic <<
	"\n maxDescriptorSetStorageBuffers: "  << m_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers <<
	"\n maxDescriptorSetStorageBuffersDynamic: "   << m_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic <<
	"\n maxDescriptorSetSampledImages: "   << m_physicalDeviceProperties.limits.maxDescriptorSetSampledImages <<
	"\n maxDescriptorSetStorageImages: "   << m_physicalDeviceProperties.limits.maxDescriptorSetStorageImages <<
	"\n maxDescriptorSetInputAttachments: "    << m_physicalDeviceProperties.limits.maxDescriptorSetInputAttachments <<
	"\n maxVertexInputAttributes: "    << m_physicalDeviceProperties.limits.maxVertexInputAttributes <<
	"\n maxVertexInputBindings: "  << m_physicalDeviceProperties.limits.maxVertexInputBindings <<
	"\n maxVertexInputAttributeOffset: "   << m_physicalDeviceProperties.limits.maxVertexInputAttributeOffset <<
	"\n maxVertexInputBindingStride: "     << m_physicalDeviceProperties.limits.maxVertexInputBindingStride <<
	"\n maxVertexOutputComponents: "   << m_physicalDeviceProperties.limits.maxVertexOutputComponents <<
	"\n maxTessellationGenerationLevel: "  << m_physicalDeviceProperties.limits.maxTessellationGenerationLevel <<
	"\n maxTessellationPatchSize: "    << m_physicalDeviceProperties.limits.maxTessellationPatchSize <<
	"\n maxTessellationControlPerVertexInputComponents: "  << m_physicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents <<
	"\n maxTessellationControlPerVertexOutputComponents: "     << m_physicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents <<
	"\n maxTessellationControlPerPatchOutputComponents: "  << m_physicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents <<
	"\n maxTessellationControlTotalOutputComponents: "     << m_physicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents <<
	"\n maxTessellationEvaluationInputComponents: "    << m_physicalDeviceProperties.limits.maxTessellationEvaluationInputComponents <<
	"\n maxTessellationEvaluationOutputComponents: "   << m_physicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents <<
	"\n maxGeometryShaderInvocations: "    << m_physicalDeviceProperties.limits.maxGeometryShaderInvocations <<
	"\n maxGeometryInputComponents: "  << m_physicalDeviceProperties.limits.maxGeometryInputComponents <<
	"\n maxGeometryOutputComponents: "     << m_physicalDeviceProperties.limits.maxGeometryOutputComponents <<
	"\n maxGeometryOutputVertices: "   << m_physicalDeviceProperties.limits.maxGeometryOutputVertices <<
	"\n maxGeometryTotalOutputComponents: "    << m_physicalDeviceProperties.limits.maxGeometryTotalOutputComponents <<
	"\n maxFragmentInputComponents: "  << m_physicalDeviceProperties.limits.maxFragmentInputComponents <<
	"\n maxFragmentOutputAttachments: "    << m_physicalDeviceProperties.limits.maxFragmentOutputAttachments <<
	"\n maxFragmentDualSrcAttachments: "   << m_physicalDeviceProperties.limits.maxFragmentDualSrcAttachments <<
	"\n maxFragmentCombinedOutputResources: "  << m_physicalDeviceProperties.limits.maxFragmentCombinedOutputResources <<
	"\n maxComputeSharedMemorySize: "  << m_physicalDeviceProperties.limits.maxComputeSharedMemorySize <<
	"\n maxComputeWorkGroupCount[0]: "    << m_physicalDeviceProperties.limits.maxComputeWorkGroupCount[0] <<
	"\n maxComputeWorkGroupInvocations: "  << m_physicalDeviceProperties.limits.maxComputeWorkGroupInvocations <<
	"\n maxComputeWorkGroupSize[0]: "     << m_physicalDeviceProperties.limits.maxComputeWorkGroupSize[0] <<
	"\n subPixelPrecisionBits: "   << m_physicalDeviceProperties.limits.subPixelPrecisionBits <<
	"\n subTexelPrecisionBits: "   << m_physicalDeviceProperties.limits.subTexelPrecisionBits <<
	"\n mipmapPrecisionBits: "     << m_physicalDeviceProperties.limits.mipmapPrecisionBits <<
	"\n maxDrawIndexedIndexValue: "    << m_physicalDeviceProperties.limits.maxDrawIndexedIndexValue <<
	"\n maxDrawIndirectCount: "    << m_physicalDeviceProperties.limits.maxDrawIndirectCount <<
	"\n maxSamplerLodBias: "       << m_physicalDeviceProperties.limits.maxSamplerLodBias <<
	"\n maxSamplerAnisotropy: "        << m_physicalDeviceProperties.limits.maxSamplerAnisotropy <<
	"\n maxViewports: "    << m_physicalDeviceProperties.limits.maxViewports <<
	"\n maxViewportDimensions[0]: "   << m_physicalDeviceProperties.limits.maxViewportDimensions[0] <<
	"\n viewportBoundsRange[0]: "     << m_physicalDeviceProperties.limits.viewportBoundsRange[0] <<
	"\n viewportSubPixelBits: "    << m_physicalDeviceProperties.limits.viewportSubPixelBits <<
	"\n minMemoryMapAlignment: "       << m_physicalDeviceProperties.limits.minMemoryMapAlignment <<
	"\n minTexelBufferOffsetAlignment: "<< m_physicalDeviceProperties.limits.minTexelBufferOffsetAlignment <<
	"\n minUniformBufferOffsetAlignment: " << m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment <<
	"\n minStorageBufferOffsetAlignment: " << m_physicalDeviceProperties.limits.minStorageBufferOffsetAlignment <<
	"\n minTexelOffset: "      << m_physicalDeviceProperties.limits.minTexelOffset <<
	"\n maxTexelOffset: "  << m_physicalDeviceProperties.limits.maxTexelOffset <<
	"\n minTexelGatherOffset: "    << m_physicalDeviceProperties.limits.minTexelGatherOffset <<
	"\n maxTexelGatherOffset: "    << m_physicalDeviceProperties.limits.maxTexelGatherOffset <<
	"\n minInterpolationOffset: "      << m_physicalDeviceProperties.limits.minInterpolationOffset <<
	"\n maxInterpolationOffset: "      << m_physicalDeviceProperties.limits.maxInterpolationOffset <<
	"\n subPixelInterpolationOffsetBits: "     << m_physicalDeviceProperties.limits.subPixelInterpolationOffsetBits <<
	"\n maxFramebufferWidth: "     << m_physicalDeviceProperties.limits.maxFramebufferWidth <<
	"\n maxFramebufferHeight: "    << m_physicalDeviceProperties.limits.maxFramebufferHeight <<
	"\n maxFramebufferLayers: "    << m_physicalDeviceProperties.limits.maxFramebufferLayers <<
	"\n framebufferColorSampleCounts: " << m_physicalDeviceProperties.limits.framebufferColorSampleCounts <<
	"\n framebufferDepthSampleCounts: "<< m_physicalDeviceProperties.limits.framebufferDepthSampleCounts <<
	"\n framebufferStencilSampleCounts: "<< m_physicalDeviceProperties.limits.framebufferStencilSampleCounts <<
	"\n framebufferNoAttachmentsSampleCounts:  "<< m_physicalDeviceProperties.limits.framebufferNoAttachmentsSampleCounts <<
	"\n maxColorAttachments: "     << m_physicalDeviceProperties.limits.maxColorAttachments <<
	"\n sampledImageColorSampleCounts: "<< m_physicalDeviceProperties.limits.sampledImageColorSampleCounts <<
	"\n sampledImageIntegerSampleCounts: " << m_physicalDeviceProperties.limits.sampledImageIntegerSampleCounts <<
	"\n sampledImageDepthSampleCounts: "<< m_physicalDeviceProperties.limits.sampledImageDepthSampleCounts <<
	"\n sampledImageStencilSampleCounts: "<< m_physicalDeviceProperties.limits.sampledImageStencilSampleCounts <<
	"\n storageImageSampleCounts: " << m_physicalDeviceProperties.limits.storageImageSampleCounts <<
	"\n maxSampleMaskWords: "  << m_physicalDeviceProperties.limits.maxSampleMaskWords <<
	"\n timestampComputeAndGraphics: "     << m_physicalDeviceProperties.limits.timestampComputeAndGraphics <<
	"\n timestampPeriod: "     << m_physicalDeviceProperties.limits.timestampPeriod <<
	"\n maxClipDistances: "    << m_physicalDeviceProperties.limits.maxClipDistances <<
	"\n maxCullDistances: "    << m_physicalDeviceProperties.limits.maxCullDistances <<
	"\n maxCombinedClipAndCullDistances: "     << m_physicalDeviceProperties.limits.maxCombinedClipAndCullDistances <<
	"\n discreteQueuePriorities: "     << m_physicalDeviceProperties.limits.discreteQueuePriorities <<
	"\n pointSizeGranularity: "        << m_physicalDeviceProperties.limits.pointSizeGranularity <<
	"\n lineWidthGranularity: "        << m_physicalDeviceProperties.limits.lineWidthGranularity <<
	"\n strictLines: "     << m_physicalDeviceProperties.limits.strictLines <<
	"\n standardSampleLocations: "     << m_physicalDeviceProperties.limits.standardSampleLocations <<
	"\n optimalBufferCopyOffsetAlignment: "<< m_physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment <<
	"\n optimalBufferCopyRowPitchAlignment: "<< m_physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment <<
	"\n nonCoherentAtomSize: " << m_physicalDeviceProperties.limits.nonCoherentAtomSize << "\n";
}

void Device::printSwapchainProperties(){
	std::cout << "\n ####### Swapchain Properties: #######" <<
	"\n Min images count: " << m_swapchainProperties.capabilities.minImageCount <<
	"\n Max images count: " << m_swapchainProperties.capabilities.maxImageCount <<
	"\n Current images extent width: " << m_swapchainProperties.capabilities.currentExtent.width <<
	"\n Current images extent height: " << m_swapchainProperties.capabilities.currentExtent.height <<
	"\n Min images extent width: " << m_swapchainProperties.capabilities.minImageExtent.width <<
	"\n Min images extent height: " << m_swapchainProperties.capabilities.minImageExtent.height <<
	"\n Max images extent width: " << m_swapchainProperties.capabilities.maxImageExtent.width <<
	"\n Max images extent height: " << m_swapchainProperties.capabilities.maxImageExtent.height <<
	"\n Max Image Array Layers: " << m_swapchainProperties.capabilities.maxImageArrayLayers <<
	"\n Supported Transforms: " << m_swapchainProperties.capabilities.supportedTransforms <<
	"\n Current Transform: " << m_swapchainProperties.capabilities.currentTransform <<
	"\n Supported Composite Alpha: " << m_swapchainProperties.capabilities.supportedCompositeAlpha <<
	"\n Supported Usage Flags: " << m_swapchainProperties.capabilities.supportedUsageFlags;

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
	// for (VkFormat format : candidates) {
	//     VkFormatProperties props;
	//     vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
	// }
}

void Device::printQueueFamilyProperties(){
	uint32_t count{};
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
	std::vector<VkQueueFamilyProperties> queueProperties(count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, queueProperties.data());

	std::cout << "\n####### Queue Family index: #######" << "\n";
	for(unsigned int i = 0; i < count; i++){
		std::cout << "At index: " << i << ": " << vk::to_string((vk::QueueFlags)queueProperties[i].queueFlags) << "\n";
	}
}
