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
	void computeAnimation();
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
	ModelLoader();
private:
	std::vector<std::string> meshNames;
	std::vector<std::string> materialNames;
};
