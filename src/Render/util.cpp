#include "util.hpp"

static bool isFileExist(const std::string& filename){
	std::ifstream file(filename);

	if(file.good()){
		file.close();
		return true;
	}
	file.close();
	return false;
}

static bool makeFile(const std::string& filename) {
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

static void writeFile(const std::string& filename, char* data, size_t size){
	std::ofstream file(filename, std::ios::binary | std::ofstream::trunc);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	std::cout << "@@@@@ write file at path: " << filename << ", with size: " << size << "\n";

	file.write(data, size);
	file.close();
}

static std::vector<uint8_t> readFile(const std::string& filename) {
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

