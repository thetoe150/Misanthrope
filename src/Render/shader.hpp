#ifndef SHADER_H
#define SHADER_H 

#include "vulkan/vulkan.hpp"
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <memory>

#include "util.hpp"

typedef struct {
	vk::ShaderModule module;
	// SpvReflectShaderModule reflection;
	std::vector<uint8_t> source;
} Shader;

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<uint8_t>& code);

class ShaderLoader {
public:
	ShaderLoader();
	void LoadShader();
private:
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> m_shaders;
};

#endif//SHADER_H 
