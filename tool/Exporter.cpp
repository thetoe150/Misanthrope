#include <cstdint>
#include <stdio.h>
#include <map>
#include "spirv/unified1/spirv.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "Exporter.h"

void testRapidJson() {
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

	rapidjson::Value object1{rapidjson::kObjectType};
	object1.AddMember("key1", "value1", allocator);
	object1.AddMember("key2", "value2", allocator);
	doc.AddMember("object1", object1, allocator);
	doc.AddMember("key3", "value3", allocator);

    // bool removed = doc.RemoveMember("object1");
	// if (removed)
	// 	printf("object1 is removed\n");
    // doc.AddMember("object1", object1, allocator);
    // // Remove object by iterator using EraseMember
    // auto it = doc.FindMember("object1");
    // if (it != doc.MemberEnd()) {
    //     doc.EraseMember(it);
	// }

	rapidjson::StringBuffer buffer; 
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	printf("output string: %s\n", buffer.GetString());
}

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

Reflection retrieveReflection(const uint32_t* spvBlob, uint32_t spvSize) {
	Reflection reflection;
	std::map<uint32_t, std::string> names;
	std::map<uint32_t, Primitive> types;
	std::map<uint32_t, uint32_t> pointerToType;

	uint32_t w = 0;
	while(w < spvSize){
		if(w == 0) {
			assert(spvBlob[w] == SpvMagicNumber);
			++w;
			continue;
		}
		if(w == 1) {
			// version is word with format
			// 0x 00-major-minor-00
			uint32_t version = spvBlob[w];
			uint32_t minor = version & 0x0000FF00; 
			minor >>= 8;
			uint32_t major = version & 0x00FF0000; 
			major >>= 16;
			printf("Spirv version: %i.%i\n", major, minor);
			++w;
			continue;
		}
		if(w >= 2 && w <= 4) {
			++w;
			continue;
		}

		uint16_t opCode = ((uint16_t*)spvBlob)[w * 2];
		uint16_t wordCount = ((uint16_t*)spvBlob)[w * 2 + 1];
		switch(opCode) {
			case SpvOpEntryPoint: {
				break;
			}
			case SpvOpName: {
				std::string name;
				for (size_t i = (w+2)*4, n = (w + wordCount)*4; i < n; i++) {
					char ch = ((char*)spvBlob)[i];
					if(ch == '\0') {
						break;
					}
					name.append(1, ch);
				}
				// printf("OpName with id: %i, named: %s\n", spvBlob[w+1], name.data());
				names.emplace(spvBlob[w+1], name);
				break;
			} 
			case SpvOpMemberName:
				break;
			case SpvOpDecorate: {
				// printf("OpDecorate with %i word, id: %i, decoration %i, value %i\n", wordCount, spvBlob[w+1], spvBlob[w+2], spvBlob[w+3]);
				uint32_t decorate = spvBlob[w+2];
				if (decorate == SpvDecorationLocation) {
					Location& l = reflection.locations[reflection.locationCount];
					l.id = spvBlob[w+1];
					l.location = (uint8_t)spvBlob[w+3];
					l.name = names[l.id];
					reflection.locationCount++;
				}
				else if (decorate == SpvDecorationBinding || decorate == SpvDecorationDescriptorSet) {
					bool first = true;
					uint32_t id = spvBlob[w+1];
					unsigned int it = 0;
					for(; it < reflection.bindingCount; it++) {
						if (reflection.bindings[it].id == id) {
							first = false;
							break;
						}
					}
					Binding& b = reflection.bindings[it];
					if (first) {
						b.id = id;
						// can do this because All OpName ops are called before all OpDecorate ops
						b.name = names[b.id];
						reflection.bindingCount++;
					}

					if (decorate == SpvDecorationBinding)
						b.binding = (uint8_t)spvBlob[w+3];
					else if (decorate == SpvDecorationDescriptorSet){
						b.set = (uint8_t)spvBlob[w+3];
						reflection.descriptorSetCount = b.set > reflection.descriptorSetCount ? b.set : reflection.descriptorSetCount;
					}

				}
				else if (decorate == SpvDecorationBlock) {
				}

				break;
			}
			case SpvOpVariable: {
				uint32_t storageClass = spvBlob[w+3];
				uint32_t pointerType = spvBlob[w+1];
				if (storageClass == SpvStorageClassInput || storageClass == SpvStorageClassOutput) {
					bool found = false;
					uint32_t id = spvBlob[w+2];
					unsigned int it = 0;
					for(; it < reflection.locationCount; it++) {
						if (reflection.locations[it].id == id) {
							found = true;
							break;
						}
					}
					if (found == false){
						reflection.locationCount++;
						reflection.locations[it].id = id;
						printf("WARNING: this id for vertex location variable %i seem to have no name\n", id);
					}

					if (storageClass == SpvStorageClassInput)
						reflection.locations[it].isInput = true;

					// at OpVariable the SpvOpTypePointer should already called
					reflection.locations[it].type = types[pointerToType[pointerType]];
				}
				else if (storageClass == SpvStorageClassUniformConstant || storageClass == SpvStorageClassUniform){ // for binding
					bool found = false;
					uint32_t id = spvBlob[w+2];
					unsigned int it = 0;
					for(; it < reflection.bindingCount; it++) {
						if (reflection.bindings[it].id == id) {
							found = true;
							break;
						}
					}
					if (found == false){
						reflection.locationCount++;
						reflection.locations[it].id = id;
						printf("WARNING: this id for descriptor binding variable %i seem to have no name\n", id);
					}

					if (storageClass == SpvStorageClassUniformConstant)
						reflection.bindings[it].type = Descriptor::SAMPLER;
					else if (storageClass == SpvStorageClassUniform)
						reflection.bindings[it].type = Descriptor::UNIFORM;

					// push constant don't have decoration for it
					// else if (storageClass == uint32_t(9))
					// 	reflection.bindings[it].type = Descriptor::PUSH_CONSTANT;
				}

				break;
			}
			case SpvOpTypePointer: {
				// the type for the pointer allready declare for this type
				uint32_t storageClass = spvBlob[w+2];
				if (storageClass == SpvStorageClassInput || storageClass == SpvStorageClassOutput)
					pointerToType.emplace(spvBlob[w+1], spvBlob[w+3]);
				break;
			}
			case SpvOpTypeStruct: {
				break;
			}
			case SpvOpTypeVector: {
				uint32_t count = spvBlob[w+3];
				if (count == 2)
					types.emplace(spvBlob[w+1], Primitive::F2);
				else if (count == 3)
					types.emplace(spvBlob[w+1], Primitive::F3);
				else if (count == 4)
					types.emplace(spvBlob[w+1], Primitive::F4);

				break;
			}
			case SpvOpTypeMatrix: {
				break;
			}
			case SpvOpTypeSampledImage: {
				break;
			}
		}

		w += wordCount;
	}
	// count = max + 1
	reflection.descriptorSetCount += 1;

	return reflection;
}

void printReflection(const Reflection& reflection) {
	printf("Descriptor Set Count %i\n", reflection.descriptorSetCount);

	printf("Location Count %i\n", reflection.locationCount);
	for(unsigned int i = 0; i < reflection.locationCount; i++) {
		const Location& loc = reflection.locations[i];
		printf("At location %i of %s, named %s, type %i (id %i)\n", loc.location, loc.isInput == 1 ? "input" : "output", loc.name.c_str(), loc.type, loc.id);
	}

	printf("Binding Count %i\n", reflection.bindingCount);
	for(unsigned int i = 0; i < reflection.bindingCount; i++) {
		const Binding& bin = reflection.bindings[i];
		printf("At binding %i of set %i, named %s, type %i (id %i)\n", bin.binding, bin.set, bin.name.c_str(), bin.type, bin.id);
	}
}
