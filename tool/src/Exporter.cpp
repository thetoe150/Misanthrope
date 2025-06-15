#include <cstdint>
#include <stdio.h>

#include "Exporter.h"

Reflection combineReflection(const char* vertexPath, const char* fragmentPath) {
	FILE *file = fopen(vertexPath, "rb");
	if (file == nullptr) {
		printf("Error occur opening gltf json file %s", vertexPath);
		return {};
	}
	uint32_t vsSpvBlob[2048];
	uint32_t vsSpvSize = fread(vsSpvBlob, sizeof(uint32_t), 2048, file);
	printf("vertex shader path: %s, with size: %i\n", vertexPath, vsSpvSize);
	Reflection vsReflection = parseSpirv(vsSpvBlob, vsSpvSize);
	fclose(file);
	printReflection(vsReflection);

	file = fopen(fragmentPath, "rb");
	if (file == nullptr) {
		printf("Error occur opening gltf json file %s", fragmentPath);
		return {};
	}
	uint32_t fsSpvBlob[2048];
	uint32_t fsSpvSize = fread(fsSpvBlob, sizeof(uint32_t), 2048, file);
	printf("fragment shader path: %s, with size: %i\n", fragmentPath, fsSpvSize);
	Reflection fsRef = parseSpirv(fsSpvBlob, fsSpvSize);
	fclose(file);
	printReflection(fsRef);

	return {};
}

std::array<Blob, 2> parseGlb(const char* glbPath) {
	FILE* file = fopen(glbPath, "rb");
	if (file == nullptr) {
		printf("Error occur opening gltf json file %s", glbPath);
		return {};
	}
	// allocate from heap to avoid stack overflow for too big files
	uint32_t* glbBlob = (uint32_t*) malloc(sizeof(uint32_t) * 2048);
	uint32_t glbSize = fread(glbBlob, sizeof(uint32_t), 2048, file);

	assert(glbSize > 0 && glbBlob[0] == 0x46546C67 /*glTF*/);
	uint32_t version = glbBlob[1];
	uint32_t fileLength = glbBlob[2];

	const uint32_t jsonChunkOffset = 3;
	uint32_t blobSize = glbBlob[jsonChunkOffset];
	Blob jsonBlob(blobSize);
	assert(glbBlob[jsonChunkOffset + 1] == 0x4E4F534A /*JSON*/);
	jsonBlob.blob = (uint8_t*)&glbBlob[jsonChunkOffset + 2];

	const uint32_t binaryChunkOffset = 5 + blobSize / sizeof(uint32_t);
	uint32_t binarySize = glbBlob[binaryChunkOffset];
	Blob binaryBlob(binarySize);
	assert(glbBlob[binaryChunkOffset + 1] == 0x004E4942 /*BIN*/);
	binaryBlob.blob = (uint8_t*)&glbBlob[binaryChunkOffset + 2];

	fclose(file);
	free(glbBlob);
	return {std::move(jsonBlob), std::move(binaryBlob)};
}

int main(int argc, char** argv) {
	Reflection reflection = combineReflection(argv[1], argv[2]);

	// std::array<Blob, 2> blobs;
	// if (argc == 4) {
	// 	blobs = parseGlb(argv[3]);
	// }
	// else if (argc == 5) {
	// 	const char* gltfJsonPath = argv[3];
	// 	FILE* file = fopen(gltfJsonPath, "rb");
	// 	if (file == nullptr) {
	// 		printf("Error occur opening gltf json file %s", gltfJsonPath);
	// 		return 1;
	// 	}
	// 	Blob jsonBlob(1024 * 1024 * 64);
	// 	uint32_t jsonWordSize = fread(jsonBlob.blob, sizeof(uint32_t), jsonBlob.size / sizeof(uint32_t), file);
	// 	jsonBlob.size = jsonWordSize * 4;
	// 	jsonBlob.fit();
	// 	printf("\ngltf json path: %s, with size: %i\n", gltfJsonPath, jsonBlob.size);
	// 	fclose(file);

	// 	const char* gltfBinPath = argv[4];
	// 	FILE* binaryFile = fopen(gltfBinPath, "rb");
	// 	if (binaryFile == nullptr) {
	// 		printf("Error occur opening gltf binary file %s", gltfBinPath);
	// 		return 1;
	// 	}
	// 	Blob binaryBlob(1024 * 1024 * 256);
	// 	uint32_t binaryWordSize = fread(binaryBlob.blob, sizeof(uint32_t), binaryBlob.size / sizeof(uint32_t), binaryFile);
	// 	binaryBlob.size = binaryWordSize * 4;
	// 	binaryBlob.fit();
	// 	printf("gltf json path: %s, with size: %i\n", gltfBinPath, binaryBlob.size);
	// 	fclose(binaryFile);
	// 	blobs[0] = std::move(jsonBlob);
	// 	blobs[1] = std::move(binaryBlob);
	// }
	// else {
	// 	assert(false && "invalid number of argument");
	// }

	// filterGltf(blobs[0].blob, blobs[0].size, blobs[1].blob, blobs[1].size);

	return 0;
}

