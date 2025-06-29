#include "util.hpp"
#include <stdint.h>

std::optional<MemoryView> LoadFile(const char* i_name) {
	FILE *file = fopen(i_name, "rb");
	if (file == nullptr) {
		return {};
	}
	uint8_t* blob = (uint8_t*) malloc(2048 * sizeof(uint8_t));
	uint32_t vsSpvSize = fread(blob, sizeof(blob[0]), 2048, file);
	MemoryView res{blob, vsSpvSize};
	fclose(file);

	return res;
}
std::optional<rapidjson::Document> ParseJsonFile(const char* i_name) {
	FILE *file = fopen(i_name, "rb");
	if (file == nullptr) {
		printf("Error opening file %s\n", i_name);
		return {};
	}
	fseek(file, 0, SEEK_END);
	uint64_t size = ftell(file);
	rewind(file);

	uint8_t* blob = (uint8_t*)malloc(size + 1);
	size_t readSize = fread(blob, sizeof(uint8_t), size, file);
	blob[readSize] = '\0';
	fclose(file);

	if (!blob) {
		printf("Error reading file %s\n", i_name);
		return {};
	}

	rapidjson::Document doc;
	rapidjson::ParseResult result = doc.Parse((const char*)blob);
	if (!result) {
		printf("JSON parse error: %s at offset %llu", rapidjson::GetParseError_En(result.Code()), result.Offset());
		return {};
	}
	free(blob);

	return doc;
}

std::optional<cgltf_data*> ParseGltfFile(const char* i_name) {
	FILE *file = fopen(i_name, "rb");
	if (file == nullptr) {
		printf("Error opening file %s\n", i_name);
		return {};
	}
	fseek(file, 0, SEEK_END);
	uint64_t size = ftell(file);
	rewind(file);

	uint8_t* blob = (uint8_t*)malloc(size);
	size_t readSize = fread(blob, sizeof(uint8_t), size, file);
	fclose(file);
	if (!blob) {
		printf("Error reading file %s\n", i_name);
		return {};
	}

	cgltf_options options = {cgltf_file_type_gltf};
	cgltf_data* gltf{nullptr};
	cgltf_result result = cgltf_parse(&options, blob, size, &gltf);
	if (result != cgltf_result_success) {
		printf("Error parse gltf file %d", result);
		return {};
	}

	free(blob);
	return gltf;
}
