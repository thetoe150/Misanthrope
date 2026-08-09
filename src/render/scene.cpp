#include "scene.hpp"

Scene::Scene(std::string i_name) {
	printf("scene name: %s\n", i_name.c_str());
	m_sceneMeta = ParseJsonFile(i_name.c_str()).value();
	initModelDesc();
	initShaderDesc();

	m_modelLoader.assignModelsToLoad(m_modelDesc);
	m_modelLoader.loadModels();

	initModels();

};

void Scene::initModels() {
	for (auto& desc : m_modelDesc) {
		if (desc.isAnimated) {
			m_models.emplace_back(Model(desc, m_modelLoader.getModel(desc.name.c_str())));
		}
		else {
			m_animatedModels.emplace_back(AnimatedModel(desc, m_modelLoader.getModel(desc.name.c_str())));
		}
	}
}

void Scene::initShaderDesc() {
}

void Scene::initModelDesc() {
	if (m_sceneMeta.HasMember("models") && m_sceneMeta["models"].IsArray()) {
		const rapidjson::Value& models = m_sceneMeta["models"];
		for (rapidjson::SizeType i = 0; i < models.Size(); i++) {
			const rapidjson::Value& model = models[i];
			Model::ModelDesc modelDesc;
			if (model.HasMember("name") && model["name"].IsString()) {
				modelDesc.name = model["name"].GetString();
			}
			if (model.HasMember("position") && model["position"].IsArray()) {
				modelDesc.initPosition.x = model["position"][0].GetFloat();
				modelDesc.initPosition.y = model["position"][1].GetFloat();
				modelDesc.initPosition.z = model["position"][2].GetFloat();
			}
			if (model.HasMember("instance") && model["instance"].IsInt()) {
				modelDesc.instanceCount = model["instance"].GetInt();
			}
			if (model.HasMember("animation") && model["animation"].IsBool()) {
				modelDesc.isAnimated = model["animation"].GetBool();
			}
			if (model.HasMember("blend") && model["blend"].IsBool()) {
				modelDesc.isBlend = model["blend"].GetBool();
			}

			if (model.HasMember("meshes") && model["meshes"].IsArray()) {
				modelDesc.meshCount = model["meshes"].GetArray().Size();
				std::vector<Mesh::MeshDesc> meshDesc;
				for (unsigned int i = 0; i < modelDesc.meshCount; i++) {
					const rapidjson::Value& mesh = model["meshes"].GetArray()[i];
					Mesh::MeshDesc desc;
					if (mesh.HasMember("animation") && mesh["animation"].IsBool()) {
						desc.isAnimated = mesh["animation"].GetBool();
					}
					if (mesh.HasMember("blend") && mesh["blend"].IsBool()) {
						// desc.isBlend = mesh["blend"].GetBool();
					}

					meshDesc.push_back(desc);
				}
				modelDesc.m_meshDesc = std::move(meshDesc);
			}

			m_modelDesc.push_back(modelDesc);
		}
	}
}
