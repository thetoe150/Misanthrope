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

ModelLoader::ModelLoader() {

};

ModelLoader::~ModelLoader() {
	for (auto& [modelName, data] : m_models) {
		cgltf_free(data);
	}
};

void ModelLoader::assignModelsToLoad(std::vector<Model::ModelDesc> i_modelDesc) {
	for (const auto& desc : i_modelDesc) {
		m_models.insert({desc.name.c_str(), nullptr});
	}
}

uint8_t ModelLoader::loadModels() {
	for (auto& [modelName, data] : m_models) {
		std::string modelPath = MODEL_PATH + modelName + "/scene.gltf";
		auto result = ParseGltfFile(modelPath.c_str());
		if (result.has_value()) {
			data = result.value();
		}
		else {
			printf("Fail to load model %s", modelName);
			return 1;
		}
	}

	return 0;
};

const cgltf_data* ModelLoader::getModel(const char* i_name) {
	return m_models[i_name];
}
