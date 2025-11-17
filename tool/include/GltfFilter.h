#include <cstdint>
#include <assert.h>
#include <array>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/prettywriter.h"
#include <rapidjson/error/en.h>

#include "cgltf/cgltf.h"

struct Blob {
	Blob() {
	};

	Blob(Blob&& i_blob) {
		size = i_blob.size;
		blob = i_blob.blob;
		i_blob.blob = nullptr;
	};

	Blob& operator=(Blob&& i_blob) {
		size = i_blob.size;
		blob = i_blob.blob;
		i_blob.blob = nullptr;

		return *this;
	};

	Blob(uint32_t i_size) {
		size = i_size;
		blob = (uint8_t*) malloc(size * sizeof(uint8_t));
	};

	~Blob() {
		if (blob != nullptr)
			free(blob);
	};

	void fit() {
		blob = (uint8_t*) realloc(blob, size);
	}

	uint8_t* blob;
	uint32_t size{0};
};

std::array<Blob, 2> parseGlb(uint32_t* glbBlob, uint32_t glbSize);
void filterGltf(uint8_t* jsonBlob, uint32_t jsonLength, uint8_t* gltfBlob, uint32_t gltfSize);
