#ifndef UTIL_H
#define UTIL_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cstdint>

struct MemoryView {
	void* data;
	uint8_t size;
};


#define CHECK_VK_RESULT(f, msg)																	\
{																								\
	if(VkResult res = f){																		\
		throw std::runtime_error(msg + vk::to_string((vk::Result)res));							\
	}																							\
}																								\


#endif//UTIL_H
