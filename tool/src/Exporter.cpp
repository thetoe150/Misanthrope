#include <cstdint>
#include <stdio.h>

#include "Exporter.h"

int main(int argc, char** argv) {
	assert(argc == 3);

	const char* vertexPath = argv[2];
	FILE *file = fopen(vertexPath, "rb");
	uint32_t vsSpvBlob[2048];
	uint32_t vsSpvSize = fread(vsSpvBlob, sizeof(uint32_t), 2048, file);
	printf("vertex shader path: %s, with size: %i\n", vertexPath, vsSpvSize);
	Reflection vsReflection = retrieveReflection(vsSpvBlob, vsSpvSize);
	fclose(file);
	printReflection(vsReflection);

	const char* fragmentPath = argv[1];
	file = fopen(fragmentPath, "rb");
	uint32_t fsSpvBlob[2048];
	uint32_t fsSpvSize = fread(fsSpvBlob, sizeof(uint32_t), 2048, file);
	printf("fragment shader path: %s, with size: %i\n", fragmentPath, fsSpvSize);
	Reflection fsRef = retrieveReflection(fsSpvBlob, fsSpvSize);
	fclose(file);
	printReflection(fsRef);

	return 0;
}

