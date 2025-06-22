#include "model.hpp"

// shader.module = createShaderModule(m_device, shader.source);

void AnimatedModel::computeAnimation(const tinygltf::Model& model) {
	for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
		auto weights = computeWeights(model, meshIdx, 0);
		// check if there is animation from gltf sampler side
		if (!weights.empty()) {
			computeMorphTargets(model, meshIdx, weights);
		}
	}
}

std::vector<float> AnimatedModel::computeWeights(const tinygltf::Model& model, unsigned int meshIdx, float deltaTime) {
	// sample animation
	assert(model.animations.size() == 1);
	const tinygltf::Animation& anims = model.animations[0];
	const std::vector<tinygltf::AnimationChannel>& channels = anims.channels;
	std::vector<tinygltf::AnimationChannel>::const_iterator channel = std::find_if(channels.begin(), channels.end(), [&model, meshIdx](tinygltf::AnimationChannel& i_channel){
		auto& node = model.nodes[i_channel.target_node];
		if(node.mesh == meshIdx)
			return true;
		return false;
	});

	if (channel == channels.end())
		return {};

	auto& sampler = anims.samplers[channel->sampler];
	const tinygltf::Accessor& inputAcc = model.accessors[sampler.input];
	const tinygltf::BufferView& inputView = model.bufferViews[inputAcc.bufferView];
	const tinygltf::Buffer& inputBuffer = model.buffers[inputView.buffer];
	const unsigned char* pInData = inputBuffer.data.data() + inputView.byteOffset + inputAcc.byteOffset;	

	m_currentAnimTime += deltaTime * CANDLE_ANIMATION_SPEED;
	if (m_currentAnimTime > inputAcc.maxValues[0])
		m_currentAnimTime -= inputAcc.maxValues[0];

	const float* inputWeights = reinterpret_cast<const float*>(pInData);
	unsigned int hi = 1;
	for (; hi < inputAcc.count; hi++) {
		if(inputWeights[hi] > m_currentAnimTime)
			break;
	}

	float ratio = (m_currentAnimTime - inputWeights[hi-1]) / (inputWeights[hi] - inputWeights[hi-1]);

	const tinygltf::Accessor& outputAcc = model.accessors[sampler.output];
	const tinygltf::BufferView& outputView = model.bufferViews[outputAcc.bufferView];
	const tinygltf::Buffer& outputBuffer = model.buffers[outputView.buffer];
	const unsigned char* pOutData = outputBuffer.data.data() + outputView.byteOffset + outputAcc.byteOffset;	

	const float* outputWeights = reinterpret_cast<const float*>(pOutData);
	const float* liWeights = outputWeights + (hi - 1) * inputAcc.count;
	const float* hiWeights = outputWeights + hi * inputAcc.count;
	
	std::vector<float> res{};
	res.resize(inputAcc.count);
	for (unsigned int i = 0; i < res.size(); i++) {
		res[i] = hiWeights[i] * ratio + liWeights[i] * (1 - ratio);
		// std::cout << "hiWeights[" << i << "]" << " = " << hiWeights[i] << "\n";
		// std::cout << "liWeights[" << i << "]" << " = " << liWeights[i] << "\n";
		// std::cout << "res[" << i << "]" << " = " << res[i] << "\n";
	}

	return res;
}

void AnimatedModel::computeMorphTargets(const tinygltf::Model& model, unsigned int meshIdx, std::vector<float> weights) {
	auto& mesh = model.meshes[meshIdx];
	// re-set to original position
	auto& attributes = mesh.primitives[0].attributes;
	const tinygltf::Accessor& posAccessor = model.accessors[attributes["POSITION"]];
	const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
	const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
	
	const unsigned char* pData = posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset;
	// NOTE: Position is NOT at the first attribute
	unsigned int posBufferIdx{0};
	for (unsigned int i = 0; i < attributes.size(); i++) {
		auto attrIt = attributes.begin();
		std::advance(attrIt, i);
		if(attrIt->first == "POSITION"){
			posBufferIdx = i;
			break;
		}
	}
	m_vertexBuffers.candles[meshIdx][posBufferIdx].needTransfer = true;
	m_vertexBuffers.candles[meshIdx][posBufferIdx].size = posAccessor.count * sizeof(glm::vec3);
	memcpy(m_vertexBuffers.candles[meshIdx][posBufferIdx].raw, pData, posAccessor.count * sizeof(glm::vec3));
	glm::vec3* pPosVec = reinterpret_cast<glm::vec3*>(m_vertexBuffers.candles[meshIdx][posBufferIdx].raw);

	// accumulate with each morph target
	auto& morphTargets = mesh.primitives[0].targets;
	for (unsigned int morphIdx = 0; morphIdx < morphTargets.size(); morphIdx++) {
		unsigned int morphAccessorIdx = morphTargets[morphIdx]["POSITION"];
		const tinygltf::Accessor& morphAccessor = model.accessors[morphAccessorIdx];
		assert(posAccessor.count == morphAccessor.count);
		const tinygltf::BufferView& bufferView = model.bufferViews[morphAccessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
		const unsigned char* pMorphData = buffer.data.data() + bufferView.byteOffset + morphAccessor.byteOffset;
		const glm::vec3* pMorphVec = reinterpret_cast<const glm::vec3*>(pMorphData);

		for (unsigned int vertexIdx = 0; vertexIdx < morphAccessor.count; vertexIdx++){
			pPosVec[vertexIdx] += pMorphVec[vertexIdx] * weights[morphIdx];
		}
	}
}

void AnimatedModel::traverseModelNodesForTransform(const tinygltf::Model& model, tinygltf::Node node, glm::mat4 mat) {
	if (node.children.empty()) {
		if (node.mesh != -1) {
			m_modelMeshTransforms[obj][node.mesh] = mat;
			// std::cout << "m_modelMeshTransforms at mesh " << node.mesh << " is:" << glm::to_string(mat) << "\n";
			return;
		}
	}
	
	if(!node.matrix.empty()) {
		glm::mat4 nodeMat = glm::make_mat4(node.matrix.data());
		// nodeMat = glm::transpose(nodeMat);
		mat = nodeMat * mat;
	} else if(!node.scale.empty() || !node.rotation.empty() || !node.translation.empty()) {
		if (!node.translation.empty()) {
			glm::vec3 translateVec = glm::make_vec3(node.translation.data());
			mat = glm::translate(mat, translateVec);
		}
		if (!node.scale.empty()) {
			glm::vec3 scaleVec = glm::make_vec3(node.scale.data());
			mat = glm::scale(mat, scaleVec);
		}
	}

	for (auto& childIdx : node.children) {
		const tinygltf::Node& child = model.nodes[childIdx];
		traverseModelNodesForTransform(model, child, mat);
	}
}
