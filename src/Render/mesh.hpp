#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "util.hpp"
#include "allocator.hpp"
#include "cgltf/cgltf.h"

// manage mesh data passed down from model and create runtime vk objects, 1 mesh / drawcall
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

class StandardMesh : IMesh {
public:
	void Build() override;
private:
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
};

// load meshes from cgltf model
class MeshLoader {
	std::unordered_map<std::string, std::shared_ptr<cgltf_mesh>> meshMetas;
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> meshes;
	cgltf_mesh ProvideMesh(std::string);
};
