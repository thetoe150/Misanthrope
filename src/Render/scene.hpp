#include "view.hpp"
#include "shader.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "util.hpp"

class Scene {
public:
	Scene(std::string);
	void initScene();
	void parseScene();

	// auto now = std::chrono::high_resolution_clock::now();
	// float currentTime = std::chrono::duration<float, std::chrono::seconds::period>(now - startTime).count();
	// m_lastTime = currentTime;

	float m_lastTime;
	float m_currentDeltaTime = 0;
	float m_currentAnimTime = 0;

    uint32_t m_currentFrame = 0;

private:
	rapidjson::Document m_sceneMeta;

	std::vector<AnimatedModel> m_animatedModels;
	std::vector<StandardModel> m_models;
	std::vector<BatchedModel> m_batchedModels;

	ModelLoader m_modelLoader;
	MeshLoader m_meshLoader;
	TextureLoader m_textureLoader;
	ShaderLoader m_shaderLoader;

	View m_view;
	ShadowView m_shadowView;
};
