#include "model.hpp"

Model::Model(ModelDesc i_desc, const cgltf_data* i_meta) 
	: m_modelDesc(i_desc), m_modelMeta(i_meta) {
}

AnimatedModel::AnimatedModel(ModelDesc i_desc, const cgltf_data* i_data)
	: Model(i_desc, i_data) {

};

void AnimatedModel::computeAnimation(cgltf_data i_data) {
};

std::vector<float> AnimatedModel::computeWeights(unsigned int meshIdx, float deltaTime) {
	return {};
};

void AnimatedModel::computeMorphTargets(unsigned int meshIdx, std::vector<float> weights) {

};

void AnimatedModel::traverseModelNodesForTransform() {

};

ModelManager::ModelManager() {

};

ModelManager::~ModelManager() {
	for (auto& [modelName, data] : m_models) {
		cgltf_free(data);
	}
};

bool ModelManager::loadModels(std::vector<std::string> i_paths) {
	for (const auto& path : i_paths) {
		std::string modelPath = path + "/scene.gltf";
		std::optional<cgltf_data*> result = ParseGltfFile(modelPath.c_str());
		if (result.has_value()) {
			cgltf_data* data = result.value();
			// should these loading happen elsewhere?
			cgltf_options options = {cgltf_file_type_gltf};
			cgltf_load_buffers(&options, data, modelPath.c_str());

			m_models.insert(std::pair(path, data));
		}
		else 
			return false;
	}

	return true;
};

const cgltf_data* ModelManager::getModel(const char* i_name) {
	return m_models[i_name];
}
