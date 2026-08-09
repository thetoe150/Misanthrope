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
#include <vector>
#include <fstream>
#include <iostream>

struct MemoryView {
	uint8_t* data{0};
	uint32_t size{0};
};

bool IsFileExist(const std::string& filename);
bool MakeFile(const std::string& filename);
void WriteFile(const std::string& filename, char* data, size_t size);
std::vector<uint8_t> ReadFile(const std::string& filename);

std::optional<rapidjson::Document> ParseJsonFile(const char* i_name);
std::optional<cgltf_data*> ParseGltfFile(const char* i_name);

#endif//UTIL_H
