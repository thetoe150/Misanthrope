#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string>

#define MAX_DESCRIPTOR_SET 4
#define MAX_BINDING 5
#define MAX_LOCALTION 10
#define MAX_PUSH_CONSTANT 3

enum class Primitive {
	I = 0,
	F,
	F2,
	F3,
	F4,
	F3x3,
	F4x4,
	STRUCT,
};

enum class Descriptor {
	UNIFORM,
	// DYNAMIC_UNIFORM???,
	// STORAGE???,
	// PUSH_CONSTANT,
	SAMPLER
};

struct Location {
	uint32_t id;
	std::string name;
	Primitive type;
	uint8_t location;
	bool isInput;
};

struct Binding {
	uint32_t id;
	std::string name;
	Descriptor type;
	uint8_t binding;
	uint8_t set;
};

struct PushConstant {
	uint32_t id;
	std::string name;
	Primitive type;
	uint8_t size;
};

struct Reflection {
	uint8_t descriptorSetCount{0};
	uint8_t locationCount{0};
	Location locations[MAX_LOCALTION];
	uint8_t bindingCount{0};
	Binding bindings[MAX_BINDING];
	uint8_t pushConstantCount{0};
	PushConstant pushConstants[MAX_PUSH_CONSTANT];
};

Reflection retrieveReflection(const uint32_t* spvBlob, uint32_t spvSize);
void printReflection(const Reflection& reflection);
