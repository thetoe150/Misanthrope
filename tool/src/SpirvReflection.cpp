#include <map>
#include <assert.h>

#include "spirv/unified1/spirv.h"
#include "SpirvReflection.h"

Semantic getLocationSemanticByName(std::string& name) {
	if (name.find("position") != std::string::npos) 
		return Semantic::POSITION;
	if (name.find("normal") != std::string::npos) 
		return Semantic::NORMAL;
	if (name.find("tangent") != std::string::npos) 
		return Semantic::TANGENT;
	if (name.find("texCoord") != std::string::npos) 
		return Semantic::TEXCOORD_0;

	return Semantic::COUNT;
}

Semantic getBindingSemanticByName(std::string& name) {
	return Semantic::COUNT;
}

Reflection parseSpirv(const uint32_t* spvBlob, uint32_t spvSize) {
	Reflection reflection;
	std::map<uint32_t, std::string> names;
	std::map<uint32_t, Type> types;
	std::map<uint32_t, uint32_t> pointerToType;

	Binding cacheBinding[MAX_DESCRIPTOR_SET * MAX_BINDING];
	uint8_t bindingCacheCount{0};

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
					l.semantic = getLocationSemanticByName(l.name);
					reflection.locationCount++;
				}
				else if (decorate == SpvDecorationBinding || decorate == SpvDecorationDescriptorSet) {
					bool first = true;
					uint32_t id = spvBlob[w+1];
					unsigned int it = 0;
					// check to avoid duplicate, an id will have both Binding + Descriptor Set Decoration
					for(; it < bindingCacheCount; it++) {
						if (cacheBinding[it].id == id) {
							first = false;
							break;
						}
					}
					Binding& b = cacheBinding[it];
					if (first) {
						b.id = id;
						assert(names.find(b.id) != names.end() && "This id have no name or OpName is called late");
						b.name = names[b.id];

						bindingCacheCount++;
					}

					if (decorate == SpvDecorationBinding) {
						b.bindingIdx = (uint8_t)spvBlob[w+3];
					}
					else if (decorate == SpvDecorationDescriptorSet){
						b.setIdx = (uint8_t)spvBlob[w+3];
						reflection.descriptorSetCount = b.setIdx + 1 > reflection.descriptorSetCount ? b.setIdx + 1 : reflection.descriptorSetCount;
					}

					if (b.setIdx != -1 && b.bindingIdx != -1) {
						reflection.descriptorSets[b.setIdx].bindings[reflection.descriptorSets[b.setIdx].bindingCount++] = b;
						reflection.totalBindingCount++;
					}
				}
				else if (decorate == SpvDecorationBlock) {
				}
				else if (decorate == SpvDecorationBufferBlock) {
				}

				break;
			}
			case SpvOpTypeArray: {
				break;
			}
			case SpvOpTypeRuntimeArray: {
				break;
			}
			case SpvOpTypeStruct: {
				uint16_t idx = 0;
				bool found = false;
				for (; idx < reflection.blockCount; idx++) {
					if(spvBlob[1] == reflection.blocks[idx].id) {
						found = true;
						break;
					}
				}
				assert(found && "Can't found block");
				Block& block = reflection.blocks[idx];
				for (uint16_t i = 2; i < wordCount; i++) {
					if(types.find(spvBlob[i]) != types.end()) {
						block.members[i-2].type = ;
					}
				}
				break;
			}
			case SpvOpTypeVector: {
				uint32_t count = spvBlob[w+3];
				if (count == 2)
					types.emplace(spvBlob[w+1], Type::F2);
				else if (count == 3)
					types.emplace(spvBlob[w+1], Type::F3);
				else if (count == 4)
					types.emplace(spvBlob[w+1], Type::F4);

				break;
			}
			case SpvOpTypeMatrix: {
				uint32_t count = spvBlob[w+3];
				// HACK: assume it's a vector of float
				if (count == 3)
					types.emplace(spvBlob[w+1], Type::F3x3);
				else if (count == 4)
					types.emplace(spvBlob[w+1], Type::F4x4);

				break;
			}
			case SpvOpTypeSampledImage: {
				break;
			}
			case SpvOpTypePointer: {
				// the type for the pointer allready declare for this type
				uint32_t storageClass = spvBlob[w+2];
				if (storageClass == SpvStorageClassInput || storageClass == SpvStorageClassOutput)
					pointerToType.emplace(spvBlob[w+1], spvBlob[w+3]);
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
					uint32_t id = spvBlob[w+2];
					bool found = false;
					unsigned int setIt = 0;
					unsigned int bindingIt = 0;
					unsigned int globalBindingIdx = 0;
					for(; setIt < reflection.descriptorSetCount; setIt++) {
						for(; bindingIt < reflection.descriptorSetCount; bindingIt++) {
							if (reflection.descriptorSets[setIt].bindings[bindingIt].id == id) {
								globalBindingIdx = reflection.descriptorSets[setIt].bindings[bindingIt].bindingIdx;
								found = true;
								break;
							}
						}
					}
					if (found == false){
						printf("WARNING: type id %i cann't found for variable \n", id);
					}

					if (storageClass == SpvStorageClassUniformConstant)
						reflection.descriptorSets[setIt].bindings[globalBindingIdx].type = Descriptor::SAMPLER;
					else if (storageClass == SpvStorageClassUniform)
						reflection.descriptorSets[setIt].bindings[globalBindingIdx].type = Descriptor::UNIFORM;

					// push constant don't have decoration for it
					// else if (storageClass == uint32_t(9))
					// 	reflection.bindings[it].type = Descriptor::PUSH_CONSTANT;
				}

				break;
			}
		}

		w += wordCount;
	}

	return reflection;
}

void printReflection(const Reflection& reflection) {
	printf("Location Count %i\n", reflection.locationCount);
	for(unsigned int i = 0; i < reflection.locationCount; i++) {
		const Location& loc = reflection.locations[i];
		printf("At location %i of %s, named %s, semantic %i, type %i (id %i)\n", loc.location, loc.isInput == 1 ? "input" : "output", loc.name.c_str(), loc.semantic, loc.type, loc.id);
	}

	printf("Descriptor Set Count %i\n", reflection.descriptorSetCount);
	printf("Binding Count %i\n", reflection.totalBindingCount);
	for(unsigned int i = 0; i < reflection.descriptorSetCount; i++) {
		for(unsigned int j = 0; j < reflection.descriptorSets[i].bindingCount; j++) {
			const Binding& bin = reflection.descriptorSets[i].bindings[j];
			printf("At binding %i of set %i, named %s, type %i (id %i)\n", bin.bindingIdx, bin.setIdx, bin.name.c_str(), bin.type, bin.id);
		}
	}
}
