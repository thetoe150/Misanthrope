#include "scene.hpp"

void Scene::initScene() {
	cgltf_result cgltf_parse_file(const cgltf_options* options, const char* path, cgltf_data** out_data);
}

void loadShader(Shader& shader, std::string path) {
}

void loadShaders() {
}

void loadInstanceData() {
	std::ifstream file("../../data/instance_position.csv");
	if(file.is_open()) {
		std::string line;
		while(std::getline(file, line)){
			VertexInstance vInstance{};
			unsigned int offset = 0;
			unsigned int space = line.find(" ");
			float x = stof(line.substr(offset, space - offset));
			offset = space + 1;
			space = line.find(" ", offset);
			float y = stof(line.substr(offset, space - offset));
			offset = space + 1;
			space = line.find(" ", offset);
			float z = stof(line.substr(offset, space - offset));
			vInstance.pos = {x, y, z};
			m_towerInstanceRaw.push_back(vInstance);
		}
		m_sceneContext.candlesShadowInstanceCount = m_towerInstanceRaw.size();
	}
}


void initUniformData() {
}

std::vector<float> interleaveAttributes(Object obj, unsigned int meshIdx) {
}
