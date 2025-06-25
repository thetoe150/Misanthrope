#include "scene.hpp"


void Scene::loadModels() {
	// trace();
	std::cout << "start loading models \n";
	// loadObjectModel(Object::TOWER);
	// loadObjectModel(Object::SNOWFLAKE);

	for (unsigned int i = 0; i < Object::COUNT; i++){
		Object objIdx = static_cast<Object>(i);
		tinygltf::Model& model = m_model[objIdx];

		std::string path{};
		if (objIdx == Object::CANDLE)
			path = CANDLE_MODEL_PATH;
		else if (objIdx == Object::SNOWFLAKE)
			path = SNOWFLAKE_MODEL_PATH;

		loadGltfModel(model, path.c_str());
		// for (unsigned int i = 0; i < model.meshes.size(); i++) {
		// 	auto& primitives = model.meshes[i].primitives;
		// 	std::cout << "primitve count: " << primitives.size() << "\n";
		// 	for(auto& attr : primitives[0].attributes) {
		// 		std::cout << "attribute " << attr.first << " - ";
		// 	}
		// 	std::cout << "attribute count: " << primitives[0].attributes.size() << "\n";
		// }

		m_modelMeshTransforms[objIdx].resize(model.meshes.size());
		traverseModelNodesForTransform(objIdx, model.nodes[0], glm::mat4(1.0f));
	}

	std::cout << "finish loading models \n";
}

void loadShader(Shader& shader, std::string path) {
	shader.source = readFile(path); 
	SpvReflectResult result = spvReflectCreateShaderModule(shader.source.size(), (void*)shader.source.data(), &shader.reflection);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
}

void loadShaders() {
}

void Scene::loadGltfModel(tinygltf::Model &model, const char *filename) {
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!warn.empty()) {
		std::cout << "WARN: " << warn << std::endl;
	}
	if (!err.empty()) {
		std::cout << "ERR: " << err << std::endl;
	}
	if (!res)
		std::cout << "Failed to load glTF: " << filename << std::endl;
	else
		std::cout << "Loaded glTF: " << filename << std::endl;
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

void initIndexData() {
	{
		tinygltf::Model& model = m_model[Object::SNOWFLAKE];
		tinygltf::Mesh& mesh = model.meshes[0];
		tinygltf::Primitive& primitive = mesh.primitives[0];
		tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
		tinygltf::BufferView& view = model.bufferViews[indexAccessor.bufferView];

		m_indexBuffers.snowflake.raw = &model.buffers[view.buffer].data.at(0) + view.byteOffset + indexAccessor.byteOffset;
		m_indexBuffers.snowflake.size = view.byteLength;
		m_indexBuffers.snowflake.needTransfer = true;
	}

	tinygltf::Model& model = m_model[Object::CANDLE];
	m_indexBuffers.candles.lod0.resize(model.meshes.size());
	for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
		const auto& mesh = model.meshes[meshIdx];
		assert(mesh.primitives.size() == 1);
		const auto& primitive = mesh.primitives[0];
		const auto& indexAcc = model.accessors[primitive.indices];
		const auto& indexView = model.bufferViews[indexAcc.bufferView];
		const auto& indexBuffer = model.buffers[indexView.buffer];

		void* data = (void*)(indexBuffer.data.data() + indexView.byteOffset + indexAcc.byteOffset);
		unsigned int size = indexAcc.count * sizeof(unsigned int);
		m_indexBuffers.candles.lod0[meshIdx].size = size;
		m_indexBuffers.candles.lod0[meshIdx].needTransfer = true;
		m_indexBuffers.candles.lod0[meshIdx].raw = malloc(size);
		memcpy(m_indexBuffers.candles.lod0[meshIdx].raw, data, size);
	}
}

void initUniformData() {
	for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_graphicUniformBuffers.shadow.perMeshTransform[i].size = sizeof(ShadowPerMeshTransform) * CANDLES_BASE_MESH_COUNT;
		m_graphicUniformBuffers.shadow.lightTransform.size = sizeof(ShadowLightingTransform);
	}

	m_graphicUniformBuffers.shadow.perInstanceTransform.size = sizeof(glm::mat4) * CANDLES_INSTANCE_MAX;
}

void initShadowData() {
	tinygltf::Model& model = m_model[Object::CANDLE];
	std::vector<float> shadowVertices;
	std::vector<unsigned int> shadowIndices;
	float shadowMeshIdx = 0;
	// candles meshes
	for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
		// only the base of the candles cast shadow
		if (m_vertexBuffers.candles[meshIdx].size() == 1) {
			const unsigned int* i = reinterpret_cast<unsigned int*>(m_indexBuffers.candles.lod0[meshIdx].raw);
			const unsigned int iCount = m_indexBuffers.candles.lod0[meshIdx].size / sizeof(unsigned int);
			const unsigned int vertexOffset = shadowVertices.size() / 4;
			for(unsigned int idx = 0; idx < iCount; idx++) {
				shadowIndices.push_back(i[idx] + vertexOffset);
			}

			const float* v = reinterpret_cast<float*>(m_vertexBuffers.candles[meshIdx][0].raw);
			const unsigned int vCount = m_vertexBuffers.candles[meshIdx][0].size / sizeof(float);
			const unsigned int stride = 12;
			// interleaved vertex data already in shader attr order
			const unsigned int offset = 0;
			for(unsigned int idx = 0 + offset; idx < vCount; idx += stride) {
				for(unsigned int vt = 0; vt < 3; vt++) {
					shadowVertices.push_back(v[idx + vt]);
				}
				shadowVertices.push_back(shadowMeshIdx);
			}
			shadowMeshIdx++;
		}
	}

	unsigned int vSize = shadowVertices.size() * sizeof(float);
	m_vertexBuffers.shadow.raw = malloc(vSize);
	memcpy(m_vertexBuffers.shadow.raw, shadowVertices.data(), vSize);
	m_vertexBuffers.shadow.size = vSize;
	m_vertexBuffers.shadow.needTransfer = true;

	unsigned int iSize = shadowIndices.size() * sizeof(unsigned int);
	m_indexBuffers.shadow.raw = malloc(iSize);
	memcpy(m_indexBuffers.shadow.raw, shadowIndices.data(), iSize);
	m_indexBuffers.shadow.size = iSize;
	m_indexBuffers.shadow.needTransfer = true;

	m_sceneContext.candlesShadowMeshCount = shadowMeshIdx;
}

std::vector<float> interleaveAttributes(Object obj, unsigned int meshIdx) {
	std::vector<float> res;
	auto& model = m_model[obj];
	auto& mesh = model.meshes[meshIdx];
	assert(mesh.primitives.size() == 1);	
	auto& attributes = mesh.primitives[0].attributes;
	unsigned int count = model.accessors[attributes["POSITION"]].count;
	res.reserve(count * 12); // 3 for pos, 3 for normal, 4 for tangent, 2 for texCoord
	
	for(unsigned int vertex_offset = 0; vertex_offset < count; vertex_offset++) {
		const SpvReflectShaderModule& reflection = m_shaders.candlesVS.reflection;
		for(unsigned int i = 0; i < reflection.input_variable_count - 1/*exclude instance buffer*/; i++){
			std::string reflectAttr = getNameAttrAtIndex(reflection, i);
			auto& modelAttr = attributes[AttrNameMap[reflectAttr]];
			auto& accessor = model.accessors[modelAttr];
			auto& bufferView = model.bufferViews[accessor.bufferView];
			auto& buffer = model.buffers[bufferView.buffer];

			assert(accessor.count == count);
			void* src = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
			float* offset_src = (float*) src + vertex_offset * accessor.type;
			for (unsigned int o = 0; o < accessor.type; o++) {
				res.push_back(offset_src[o]);
			}
		}
	}

	return res;
}
