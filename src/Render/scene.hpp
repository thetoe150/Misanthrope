#include "view.hpp"
#include "shader.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "shader.hpp"

struct {
	Shader snowflakeVS;
	Shader snowflakeFS;
	Shader snowflakeCS;

	Shader candlesVS;
	Shader candlesFS;

	Shader skyboxVS;
	Shader skyboxFS;

	Shader floorVS;
	Shader floorFS;

	Shader quadVS;
	Shader bloomFS;
	Shader combineFS;
	Shader shadowViewportFS;

	Shader shadowBatchVS;
} m_shaders;

class Scene {
private:
	ModelLoader m_modelLoader;
	MeshLoader m_meshLoader;
	MaterialLoader m_materialLoader;
	ShaderLoader m_shaderLoader;

	View m_view;
	ShadowView m_shadowView;
public:
	void loadModels();
	void loadShaders();
	void initVertexData();
	void computeAnimation();
	void loadGltfModel(tinygltf::Model &model, const char *filename);

	void initIndexData();
	// analyzeMeshes(false);
	void optimizeMeshes();
	// shadow don't have LOD
	void generateIndexLOD();

	void initShadowData();

	void initSceneContext();
	void initUniformData();
	// analyzeMeshes(true);

	// auto now = std::chrono::high_resolution_clock::now();
	// float currentTime = std::chrono::duration<float, std::chrono::seconds::period>(now - startTime).count();
	// m_lastTime = currentTime;

	float m_lastTime;
	float m_currentDeltaTime = 0;
	float m_currentAnimTime = 0;

    uint32_t m_currentFrame = 0;
};
