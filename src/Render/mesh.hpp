#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "util.hpp"
#include "allocator.hpp"
#include "cgltf/cgltf.h"

// manage model mesh and create runtime vk objects, 1 mesh / drawcall
struct IMesh {
	Slot indexBuffer;
	Slot vertexBuffer;
};

struct AnimatedMesh : IMesh {
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
	MemoryView morphTarget;
};

struct BatchedMesh : IMesh {
	std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
	MemoryView mesh;
};

struct StandardMesh : IMesh {
	std::shared_ptr<cgltf_mesh> meshMeta;
	MemoryView mesh;
};

// load meshes from cgltf model
class MeshLoader {
	std::unordered_map<std::string, std::shared_ptr<cgltf_mesh>> meshMetas;
	std::unordered_map<std::string, std::shared_ptr<MemoryView>> meshes;
	cgltf_mesh ProvideMesh(std::string);
};
