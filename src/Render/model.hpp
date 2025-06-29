#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

#include "cgltf/cgltf.h"

#include "mesh.hpp"
#include "material.hpp"
#include "global.hpp"

class IModel {
public:
	virtual void build() = 0;
	virtual std::vector<std::string> gatherMeshesToLoad() = 0;
	virtual std::vector<std::string> gatherTexturesToLoad() = 0;
protected:
	cgltf_data* m_modelMeta{nullptr};
	std::string m_name;
	glm::vec3 m_position;
};

class AnimatedModel : IModel {
public:
	AnimatedModel(const rapidjson::Value&);

	void build() override;
	std::vector<std::string> gatherMeshesToLoad() override;
	std::vector<std::string> gatherTexturesToLoad() override;

	void computeAnimation(cgltf_data i_data);
	std::vector<float> computeWeights(unsigned int meshIdx, float deltaTime);
	void computeMorphTargets(unsigned int meshIdx, std::vector<float> weights);
	void traverseModelNodesForTransform();

private:
	float m_currentAnimTime;
	std::vector<AnimatedMesh> meshes;
};

class StandardModel : IModel {
public:
	StandardModel(const rapidjson::Value&);
	std::vector<std::string> gatherMeshesToLoad() override;
	std::vector<std::string> gatherTexturesToLoad() override;
	void build() override;

private:
	StandardMesh mesh;
};

class BatchedModel : IModel {
public:
	BatchedModel(const rapidjson::Value&);

private:
	BatchedMesh mesh;
};

class ModelLoader {
public:
	ModelLoader();
	ModelLoader(std::vector<std::string> names);
	uint8_t LoadModel(const char* i_name);
	const cgltf_data* getModel(const char* i_name);

private:
	std::unordered_map<const char*, const cgltf_data*> m_loadedModels;
};

#endif//MODEL_H
