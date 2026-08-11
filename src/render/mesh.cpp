#include "mesh.hpp"

#include "renderer.hpp"

MeshLoader::MeshLoader() {}

uint8_t MeshLoader::loadMeshes(std::vector<cgltf_data*> i_meshes) {
    cgltf_options options;
    for (const auto& mesh : i_meshes) {
        // cgltf_load_buffers(options, mesh, path);
    }

    return {};
}

void Mesh::acquireGpuResource() {}

void Mesh::gatherPipelineDesc() {}

void StaticMesh::initVertexData() {
    cgltf_primitive& primitive = m_meshData->primitives[0];
    cgltf_accessor* posAccessor = getAccessorForAttr(primitive, cgltf_attribute_type_position);
    float* position = reinterpret_cast<float*>(getBufferPointerFromAccessor(posAccessor));

    m_vertexBuffer.cpuBuffer.data = position;
    m_vertexBuffer.cpuBuffer.size = posAccessor->count * sizeof(glm::vec3);
}

void StaticMesh::createVertexBuffer() {
    m_vertexBuffer.gpuBuffer =
        m_gpuAllocator->allocateDeviceBuffer(m_vertexBuffer.cpuBuffer, BufferType::VERTEX_BUFFER);
    m_gpuAllocator->map(m_vertexBuffer);
}

void StaticMesh::initIndexData() {
    cgltf_primitive& primitive = m_meshData->primitives[0];
    cgltf_accessor* accessor = primitive.indices;

    m_indexBuffers[0].cpuBuffer.data = getBufferPointerFromAccessor(accessor);
    m_indexBuffers[0].cpuBuffer.size = accessor->buffer_view->size;
}

void StaticMesh::createIndexBuffer() {
    m_indexBuffers[0].gpuBuffer =
        m_gpuAllocator->allocateDeviceBuffer(m_indexBuffers[0].cpuBuffer, BufferType::INDEX_BUFFER);
    m_gpuAllocator->map(m_indexBuffers[0]);
}

void StaticMesh::createUniformBuffer() {}

void AnimatedMesh::traverseModelNodesForTransform(const cgltf_node* node, glm::mat4 mat) {
    // leaf node
    if (node->children == nullptr) {
        if (node->mesh != nullptr) {
            int mesh_idx = cgltf_mesh_index(m_model, node->mesh);
            m_transform = mat;
            // std::cout << "m_modelMeshTransforms at mesh " << node.mesh << " is:" <<
            // glm::to_string(mat) << "\n";
            return;
        }
    }

    if (node->has_matrix) {
        glm::mat4 nodeMat = glm::make_mat4(node->matrix);
        // nodeMat = glm::transpose(nodeMat);
        mat = nodeMat * mat;
    } else {
        if (node->has_translation) {
            glm::vec3 translateVec = glm::make_vec3(node->translation);
            mat = glm::translate(mat, translateVec);
        }
        if (node->has_scale) {
            glm::vec3 scaleVec = glm::make_vec3(node->scale);
            mat = glm::scale(mat, scaleVec);
        }
    }

    for (unsigned int i = 0; i < node->children_count; i++) {
        const cgltf_node* child = node->children[i];
        traverseModelNodesForTransform(child, mat);
    }
}

std::vector<float> AnimatedMesh::computeFrameWeights(unsigned int meshIdx, float deltaTime) {
    std::vector<float> res{};

    assert(m_model->animations_count == 1);
    const cgltf_animation* anims = &m_model->animations[0];
    const cgltf_animation_channel* channels = anims->channels;

    const cgltf_animation_channel* desired_channel = nullptr;
    for (unsigned int i = 0; i < anims->channels_count; i++) {
        cgltf_mesh* mesh = channels[i].target_node->mesh;
        if (cgltf_mesh_index(m_model, mesh) == meshIdx) {
            desired_channel = &channels[i];
            break;
        }
    }
    if (desired_channel == nullptr) return res;

    cgltf_animation_sampler* sampler = desired_channel->sampler;
    const cgltf_accessor* inputAcc = sampler->input;
    const unsigned char* pInData = getBufferPointerFromAccessor(inputAcc);

    m_currentAnimTime += m_currentDeltaTime * CANDLE_ANIMATION_SPEED;
    if (m_currentAnimTime > inputAcc->max[0]) m_currentAnimTime -= inputAcc->max[0];

    const float* inputWeights = reinterpret_cast<const float*>(pInData);
    unsigned int hi = 1;
    for (; hi < inputAcc->count; hi++) {
        if (inputWeights[hi] > m_currentAnimTime) break;
    }

    float ratio =
        (m_currentAnimTime - inputWeights[hi - 1]) / (inputWeights[hi] - inputWeights[hi - 1]);

    const cgltf_accessor* outputAcc = sampler->output;
    const unsigned char* pOutData = getBufferPointerFromAccessor(outputAcc);

    const float* outputWeights = reinterpret_cast<const float*>(pOutData);
    const float* liWeights = outputWeights + (hi - 1) * inputAcc->count;
    const float* hiWeights = outputWeights + hi * inputAcc->count;

    res.resize(outputAcc->count);
    for (unsigned int i = 0; i < res.size(); i++) {
        res[i] = hiWeights[i] * ratio + liWeights[i] * (1 - ratio);
        // std::cout << "hiWeights[" << i << "]" << " = " << hiWeights[i] << "\n";
        // std::cout << "liWeights[" << i << "]" << " = " << liWeights[i] << "\n";
        // std::cout << "res[" << i << "]" << " = " << res[i] << "\n";
    }

    return res;
};

void AnimatedMesh::computeFrameMorphTargets(unsigned int meshIdx, std::vector<float> weights) {
    const cgltf_mesh* mesh = &m_model->meshes[meshIdx];
    // re-set to original position
    //
    cgltf_accessor* pos_acc =
        getAccessorForAttr(mesh->primitives[0], cgltf_attribute_type_position);
    const unsigned char* posData = getBufferPointerFromAccessor(pos_acc);

    // NOTE: Position is NOT at the first attribute
    unsigned int posBufferIdx{0};
    for (unsigned int i = 0; i < attributes.size(); i++) {
        auto attrIt = attributes.begin();
        std::advance(attrIt, i);
        if (attrIt->first == "POSITION") {
            posBufferIdx = i;
            break;
        }
    }

    m_positionBuffer m_vertexBuffers.candles[meshIdx][posBufferIdx].needTransfer = true;
    m_vertexBuffers.candles[meshIdx][posBufferIdx].size = posAccessor.count * sizeof(glm::vec3);
    memcpy(m_vertexBuffers.candles[meshIdx][posBufferIdx].raw, pData,
           posAccessor.count * sizeof(glm::vec3));
    glm::vec3* pPosVec =
        reinterpret_cast<glm::vec3*>(m_vertexBuffers.candles[meshIdx][posBufferIdx].raw);

    // accumulate with each morph target
    auto& morphTargets = mesh.primitives[0].targets;
    for (unsigned int morphIdx = 0; morphIdx < morphTargets.size(); morphIdx++) {
        unsigned int morphAccessorIdx = morphTargets[morphIdx]["POSITION"];
        const tinygltf::Accessor& morphAccessor = model.accessors[morphAccessorIdx];
        assert(posAccessor.count == morphAccessor.count);
        const tinygltf::BufferView& bufferView = model.bufferViews[morphAccessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        const unsigned char* pMorphData =
            buffer.data.data() + bufferView.byteOffset + morphAccessor.byteOffset;
        const glm::vec3* pMorphVec = reinterpret_cast<const glm::vec3*>(pMorphData);

        for (unsigned int vertexIdx = 0; vertexIdx < morphAccessor.count; vertexIdx++) {
            pPosVec[vertexIdx] += pMorphVec[vertexIdx] * weights[morphIdx];
        }
    }
};

cgltf_accessor* getAccessorForAttr(cgltf_primitive& i_primitive, cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i_primitive.attributes[i].data;
        }
    }

    return nullptr;
}

cgltf_accessor* getAccessorForAttr(cgltf_morph_target& i_primitive, cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i_primitive.attributes[i].data;
        }
    }

    return nullptr;
}

int getIndexForAttr(cgltf_primitive& i_primitive, cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i;
        }
    }

    return -1;
}

cgltf_attribute_type getModelAttributeFromShaderAttribute(std::string i_attributeName) {
    if (i_attributeName == "a_position") return cgltf_attribute_type_position;
    if (i_attributeName == "a_normal") return cgltf_attribute_type_normal;
    if (i_attributeName == "a_tangent") return cgltf_attribute_type_tangent;
    if (i_attributeName == "a_texCoord") return cgltf_attribute_type_texcoord;

    return cgltf_attribute_type_invalid;
}

std::string getNameAttrAtIndex(const SpvReflectShaderModule* module, uint8_t idx) {
    for (unsigned int i = 0; i < module->input_variable_count; i++) {
        if (module->input_variables[i]->location == idx) return module->input_variables[i]->name;
    }
    return "";
}

unsigned char* getBufferPointerFromAccessor(const cgltf_accessor* i_accessor) {
    unsigned char* pBuffer = (unsigned char*)i_accessor->buffer_view->buffer->data;
    size_t offset = i_accessor->offset + i_accessor->buffer_view->offset;
    return pBuffer + offset;
}

float* allocateFloatBufferForAccessor(const cgltf_accessor* i_accessor) {
    size_t count = cgltf_accessor_unpack_floats(i_accessor, nullptr, 0);
    float* buffer = (float*)malloc(count * sizeof(float));
    count = cgltf_accessor_unpack_floats(i_accessor, buffer, count);
    assert(i_accessor->buffer_view->size == count * sizeof(float));
    return buffer;
}
