#ifndef ALLOCATOR_H_INCLUDED
#define ALLOCATOR_H_INCLUDED 

#include <memory>
#include "vulkan/vulkan.hpp"
#include "vma/vk_mem_alloc.h"

struct Slot {
	VkBuffer buffer;
	size_t offset;
	size_t size;
};

template <typename BufferType>
class Allocator {

};

#endif /* #ifndef ALLOCATOR_H_INCLUDED */
