#ifndef UTIL_H
#define UTIL_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "cgltf/cgltf.h"

#include <cstdint>
#include <optional>

struct MemoryView {
	uint8_t* data{0};
	uint32_t size{0};
};

std::optional<MemoryView> LoadFile(const char*);
std::optional<rapidjson::Document> ParseJsonFile(const char* i_name);
std::optional<cgltf_data*> ParseGltfFile(const char* i_name);

#define CHECK_VK_RESULT(f, msg)																	\
{																								\
	if(VkResult res = f){																		\
		throw std::runtime_error(msg + vk::to_string((vk::Result)res));							\
	}																							\
}																								\


#endif//UTIL_H
