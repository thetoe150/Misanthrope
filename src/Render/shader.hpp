#include "vulkan/vulkan.h"
#include <vector>
#include <stdexcept>

typedef struct {
	VkShaderModule module;
	// SpvReflectShaderModule reflection;
	std::vector<uint8_t> source;
} Shader;

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code);
