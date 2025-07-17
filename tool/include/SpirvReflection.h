#include <stdint.h>
#include <string>
#include "shader.hpp"

Reflection parseSpirv(const uint32_t* spvBlob, uint32_t spvSize);
void printReflection(const Reflection& reflection);

struct TmpDescriptor {
	uint32_t id;
	int8_t bindingIdx;
	int8_t setIdx;
};

struct DescriptorTable {
};
