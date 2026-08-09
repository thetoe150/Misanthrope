#include "util.hpp"
#include <stdint.h>

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

bool IsFileExist(const std::string& filename){
	std::ifstream file(filename);

	if(file.good()){
		file.close();
		return true;
	}
	file.close();
	return false;
}

bool MakeFile(const std::string& filename) {
	std::ofstream file(filename, std::ios::ate | std::ios::binary);

	if(file.good()){
		file.flush();
		file.close();
		std::cout << "@@@@@ create file at path: " << filename << "\n";
		return true;
	}
	file.close();
	std::cout << "@@@@@ FAIL to create file at path: " << filename << "\n";
	return false;
}

void WriteFile(const std::string& filename, char* data, size_t size){
	std::ofstream file(filename, std::ios::binary | std::ofstream::trunc);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	std::cout << "@@@@@ write file at path: " << filename << ", with size: " << size << "\n";

	file.write(data, size);
	file.close();
}

std::vector<uint8_t> ReadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file - " + filename);
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<uint8_t> buffer(fileSize);

	std::cout << "@@@@@ read file at path: " << filename << ", with size: " << fileSize << "\n";
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);

	file.close();

	return buffer;
}
