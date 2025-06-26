#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cgltf/cgltf.h"

#include "mesh.hpp"
#include "material.hpp"

class IModel {
	cgltf_data metadata;
};

class AnimatedModel : IModel {
public:
	void computeAnimation(cgltf_data i_data);
	std::vector<float> computeWeights(unsigned int meshIdx, float deltaTime);
	void computeMorphTargets(unsigned int meshIdx, std::vector<float> weights);
	void traverseModelNodesForTransform();

private:
	float m_currentAnimTime;
	std::vector<AnimatedMesh> meshes;
};

class BatchedModel : IModel {
public:

private:
	BatchedMesh mesh;
};

class ModelLoader {
public:
	ModelLoader(std::vector<std::string> names);
	void LoadModels();
private:
	std::shared_ptr<cgltf_data> LoadModel(std::string);
	std::vector<std::string> m_modelnames;
};

#endif//MODEL_H
