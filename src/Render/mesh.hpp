#ifndef MESH_H
#define MESH_H 

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>

#include "util.hpp"
#include "allocator.hpp"
#include "cgltf/cgltf.h"
#include "meshoptimizer.h"

// manage mesh data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class IMesh {
public:
	virtual void Build() = 0;
private:
	Slot indexBuffer;
	Slot vertexBuffer;
};

class AnimatedMesh : IMesh {
public:
	void Build() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
	MemoryView morphTarget;
};

class BatchedMesh : IMesh {
public:
	void Build() override;
private:
	std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
	MemoryView mesh;
};

class InstancedMesh : IMesh {
public:
	void Build() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
	MemoryView instanceData;
};

class BatchedInstancedMesh : IMesh {
public:
	void Build() override;
private:
	std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
	MemoryView mesh;
};

class StandardMesh : IMesh {
public:
	void Build() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
};

// load meshes from files and provide them for models do processing
class MeshLoader {
	std::unordered_map<std::string, std::shared_ptr<cgltf_mesh>> meshMetas;
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> meshes;
	cgltf_mesh ProvideMesh(std::string);
};

struct VertexInstance {
	alignas(16) glm::vec3 pos;

	static VkVertexInputBindingDescription getBindingDescription(){
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 4;
		bindingDescription.stride = sizeof(VertexInstance);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions(){
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
		attributeDescriptions[0].binding = 4;
		attributeDescriptions[0].location = 4;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		// attributeDescriptions[0].offset = offsetof(VertexInstance, pos);
		attributeDescriptions[0].offset = 0;

		return attributeDescriptions;
	}

	bool operator==(const VertexInstance& other) const{
		return pos == other.pos;
	}
};


#endif//MESH_H
