#include "mesh.hpp"

MeshLoader::MeshLoader() {

}

uint8_t MeshLoader::loadMeshes(std::vector<cgltf_data*> i_meshes) {
	cgltf_options options;
	for(const auto& mesh : i_meshes) {
		cgltf_load_buffers(options, mesh, path);
	}
}

void initIndexData() {
}

void initShadowData() {
}

void analyzeMeshes(bool isLOD) {
}
