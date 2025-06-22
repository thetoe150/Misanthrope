#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf/tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class AnimatedModel {
public:
void computeAnimation(const tinygltf::Model& model);
std::vector<float> computeWeights(const tinygltf::Model& model, unsigned int meshIdx, float deltaTime);
void computeMorphTargets(const tinygltf::Model& model, unsigned int meshIdx, std::vector<float> weights);
void traverseModelNodesForTransform(const tinygltf::Model& model, tinygltf::Node node, glm::mat4 mat);

private:
	float m_currentAnimTime;
};

class BatchedModel {
public:

private:
};
