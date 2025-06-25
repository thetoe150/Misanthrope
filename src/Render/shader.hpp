#include "vulkan/vulkan.h"
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <memory>

#include "util.hpp"

typedef struct {
	VkShaderModule module;
	// SpvReflectShaderModule reflection;
	std::vector<uint8_t> source;
} Shader;

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code);

class ShaderLoader {
public:
	void LoadShader();
private:
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> m_shaders;
};
