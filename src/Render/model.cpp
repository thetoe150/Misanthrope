#include "model.hpp"

void AnimatedModel::computeAnimation(cgltf_data i_data) {
};

std::vector<float> AnimatedModel::computeWeights(unsigned int meshIdx, float deltaTime) {

};

void AnimatedModel::computeMorphTargets(unsigned int meshIdx, std::vector<float> weights) {

};

void AnimatedModel::traverseModelNodesForTransform() {

};

std::shared_ptr<cgltf_data> ModelLoader::LoadModel(std::string i_modelName) {

};
