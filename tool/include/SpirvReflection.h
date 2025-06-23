#include <stdint.h>
#include <string>

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

	COUNT,
};

enum class BlockType {
	STRUCT,
	ARRAY_OF_STRUCT,
	RUNTIME_ARRAY_OF_STRUCT,

	COUNT,
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

enum class Descriptor {
	UNIFORM,
	// DYNAMIC_UNIFORM???,
	// STORAGE???,
	// PUSH_CONSTANT,
	SAMPLER,
	COUNT,
};

struct Location {
	uint32_t id;
	std::string name;
	Semantic semantic;
	Type type;
	uint8_t location;
	bool isInput;
};

struct Binding {
	uint32_t id;
	std::string name;
	Descriptor type{Descriptor::COUNT};
	int8_t bindingIdx{-1};
	int8_t setIdx{-1};
};

struct PushConstant {
	uint32_t id;
	std::string name;
	Type type;
	uint8_t size;
};

struct DescriptorSet {
	uint8_t bindingCount{0};
	Binding bindings[MAX_BINDING];
};

struct BlockMember {
	std::string name;
	Type type;
	uint8_t offset;
};

struct Block {
	uint32_t id;
	uint8_t memberCount{0};
	BlockMember members[MAX_MEMBER];
	BlockType type;
	uint32_t arraySize;
	uint8_t stride;

	uint8_t setIdx;
	uint8_t bindingIdx;
};

struct Sampler {
	uint8_t setIdx;
	uint8_t bindingIdx;
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

Reflection parseSpirv(const uint32_t* spvBlob, uint32_t spvSize);
void printReflection(const Reflection& reflection);
