#include "scene.hpp"

Scene::Scene(std::string i_name) {
	printf("scene name: %s\n", i_name.c_str());
	m_sceneMeta = ParseJsonFile(i_name.c_str()).value();

	if (m_sceneMeta.HasMember("models") && m_sceneMeta["models"].IsArray()) {
		const rapidjson::Value& models = m_sceneMeta["models"];
		for (rapidjson::SizeType i = 0; i < models.Size(); i++) {
			const rapidjson::Value& model = models[i];
			if (model.HasMember("animation") && model["animation"].IsString()) {
				m_animatedModels.emplace_back(AnimatedModel(model));
			}
			else {
				m_models.emplace_back(StandardModel(model));
			}
		}
	}
};

void Scene::initScene() {
}
