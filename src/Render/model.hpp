#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "robin_map/robin_map.h"

#include "cgltf/cgltf.h"

#include "mesh.hpp"
#include "material.hpp"
#include "global.hpp"

class Model {
public:
	struct ModelDesc {
		std::string name;
		uint32_t meshCount;
		uint32_t instanceCount;
		uint32_t isAnimated;
		glm::vec3 initPosition;
		bool isBlend;
		std::vector<Mesh::MeshDesc> m_meshDesc;
	};

	Model(ModelDesc, const cgltf_data*);
	virtual void build();
	virtual std::vector<std::string> gatherMeshesToLoad();
	virtual std::vector<std::string> gatherTexturesToLoad();

	ModelDesc m_modelDesc;
	const cgltf_data* m_modelMeta{nullptr};
	glm::vec3 m_position;

protected:
	std::vector<Mesh> m_meshes;
};

class AnimatedModel : Model {
public:
	AnimatedModel(ModelDesc, const cgltf_data*);

	void build() override;
	std::vector<std::string> gatherMeshesToLoad() override;
	std::vector<std::string> gatherTexturesToLoad() override;

	void computeAnimation(cgltf_data i_data);
	std::vector<float> computeWeights(unsigned int meshIdx, float deltaTime);
	void computeMorphTargets(unsigned int meshIdx, std::vector<float> weights);
	void traverseModelNodesForTransform();

private:
	float m_currentAnimTime;
	std::vector<AnimatedMesh> m_animatedMeshes;
};

class BatchedModel : Model {
public:
	BatchedModel(const rapidjson::Value&);

private:
	BatchedMesh mesh;
};

class ModelLoader {
public:
	ModelLoader();
	~ModelLoader();
	void assignModelsToLoad(std::vector<Model::ModelDesc>);
	uint8_t loadModels();
	const cgltf_data* getModel(const char* i_name);

private:
	tsl::robin_map<const char*, cgltf_data*> m_models;
};

#endif//MODEL_H
