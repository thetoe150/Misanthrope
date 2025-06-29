#include "model.hpp"

AnimatedModel::AnimatedModel(const rapidjson::Value& i_value) {
	if (i_value.HasMember("name") && i_value["name"].IsString()) {
		m_name = i_value["name"].GetString();
	}

	if (i_value.HasMember("position") && i_value["position"].IsArray()) {
		m_position.x = i_value["position"][0].GetFloat();
		m_position.x = i_value["position"][1].GetFloat();
		m_position.x = i_value["position"][2].GetFloat();
	}
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

uint8_t ModelLoader::LoadModel(const char* i_name) {
	std::string modelPath = MODEL_PATH + i_name + "/scene.gltf";
	auto result = ParseGltfFile(modelPath.c_str());
	if (result.has_value()) {
		m_loadedModels[i_name] = result.value();
		return 0;
	}
	return 1;
};

const cgltf_data* ModelLoader::getModel(const char* i_name) {
	return m_loadedModels[i_name];
}
