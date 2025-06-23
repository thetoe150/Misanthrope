#include <memory>
#include "vulkan/vulkan.h"

struct Slot {
	std::shared_ptr<VkBuffer> buffer;
	size_t offset;
	size_t size;
};

template <typename BufferType>
class Allocator {

};
