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
#include "material.hpp"

// manage mesh data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class Mesh {
public:
	struct MeshDesc {
		std::string name;
		uint32_t meshCount;
		uint32_t instanceCount;
		bool isAnimated;
		Material material;
	};
	virtual void acquireGpuResource();
private:
	MeshDesc m_meshDesc;

	Slot indexBuffer;
	Slot vertexBuffer;
};

class AnimatedMesh : Mesh {
public:
	void acquireGpuResource() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
	MemoryView morphTarget;
};

class BatchedMesh : Mesh {
public:
	void acquireGpuResource() override;
private:
	std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
	MemoryView mesh;
};

class InstancedMesh : Mesh {
public:
	void acquireGpuResource() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
	MemoryView instanceData;
};

class BatchedInstancedMesh : Mesh {
public:
	void acquireGpuResource() override;
private:
	std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
	MemoryView mesh;
};

class StandardMesh : Mesh {
public:
	void acquireGpuResource() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
};

// load meshes from files and provide them for models do processing
class MeshLoader {
public:
	MeshLoader();
	uint8_t loadMeshes(std::vector<cgltf_data*>);
	cgltf_mesh ProvideMesh(std::string);
private:
	std::unordered_map<std::string, std::shared_ptr<cgltf_mesh>> meshMetas;
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> meshes;
};

struct VertexInstance {

	static vk::VertexInputBindingDescription getBindingDescription(){
		vk::VertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 4;
		bindingDescription.stride = sizeof(VertexInstance);
        bindingDescription.inputRate = vk::VertexInputRate::eInstance;

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions(){
		std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};
		attributeDescriptions[0].binding = 4;
		attributeDescriptions[0].location = 4;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		// attributeDescriptions[0].offset = offsetof(VertexInstance, pos);
		attributeDescriptions[0].offset = 0;

		return attributeDescriptions;
	}
};


#endif//MESH_H
