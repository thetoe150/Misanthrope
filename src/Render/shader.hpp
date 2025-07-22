#ifndef SHADER_H
#define SHADER_H 

#include "vulkan/vulkan.hpp"
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <memory>

#include "util.hpp"

#define MAX_DESCRIPTOR_SET 4
#define MAX_BINDING 5
#define MAX_LOCALTION 20
#define MAX_PUSH_CONSTANT 3
#define MAX_BUFFER 8
#define MAX_MEMBER 8

enum class Type {
	I = 0,
	F,
	F2,
	F3,
	F4,
	F3x3,
	F4x4,

	COUNT
};

enum class Stage : uint8_t {
	VERTEX = (1 << 0),
	FRAGMENT = (1 << 1),

	COUNT
};

enum class BlockType {
	STRUCT,
	ARRAY_OF_STRUCT,
	RUNTIME_ARRAY_OF_STRUCT,

	COUNT
};

enum class Semantic {
	POSITION,
	NORMAL,
	TANGENT,
	COLOR,
	TEXCOORD_0,
	TEXCOORD_1,
	TEXCOORD_2,

	COUNT
};

enum class BindingType {
	UNIFORM,
	DYNAMIC_UNIFORM,
	STORAGE,
	PUSH_CONSTANT,
	SAMPLER,

	COUNT,
};

struct Location {
	std::string name;
	Semantic semantic;
	Type type;
	uint8_t location;
	bool isInput;
};

struct Binding {
	std::string name;
	BindingType type{BindingType::COUNT};

	union BindingDesc {
		int8_t blockIdx;
		int8_t samplerIdx{-1};
	};
	BindingDesc descIdx;
};

struct PushConstant {
	std::string name;
	Type type;
	uint8_t size;
	Stage stage;
};

struct DescriptorSet {
	uint8_t bindingCount{0};
	BindingType bindings[MAX_BINDING];
	Stage stage;
};

struct BlockMember {
	std::string name;
	Type type;
	uint8_t offset;
};

struct Block {
	BlockType type;
	uint8_t memberCount{0};
	BlockMember members[MAX_MEMBER];
	uint32_t arraySize;
	uint8_t stride;
};

struct Sampler {
};

struct Reflection {
	uint8_t locationCount{0};
	Location locations[MAX_LOCALTION];

	uint8_t descriptorSetCount{0};
	DescriptorSet descriptorSets[MAX_DESCRIPTOR_SET];
	uint8_t totalBindingCount{0};

	uint8_t pushConstantCount{0};
	PushConstant pushConstants[MAX_PUSH_CONSTANT];

	uint8_t blockCount{0};
	Block blocks[MAX_BUFFER];

	uint8_t samplerCount{0};
	Sampler sampler[MAX_BUFFER];
};

class Shader {
	vk::ShaderModule m_module;
	Reflection m_reflection;
	std::vector<uint8_t> m_blob;
};

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<uint8_t>& code);

class ShaderLoader {
public:
	ShaderLoader();
	void LoadShader();
private:
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> m_shaders;
};

#endif//SHADER_H 
