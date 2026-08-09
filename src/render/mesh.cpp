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
    unsigned char* pBuffer = (unsigned char*)posAccessor->buffer_view->buffer->data;
    size_t offset = posAccessor->offset + posAccessor->buffer_view->offset;
    float* position = reinterpret_cast<float*>(pBuffer + offset);

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

void AnimatedMesh::traverseModelNodesForTransform(cgltf_data* i_model, const cgltf_node* node,
                                                  glm::mat4 mat) {
    // leaf node
    if (node->children == nullptr) {
        if (node->mesh != nullptr) {
            int mesh_idx = cgltf_mesh_index(i_model, node->mesh);
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
        traverseModelNodesForTransform(i_model, child, mat);
    }
}

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

cgltf_attribute_type getModelAttributeForShaderAttribute(std::string i_attributeName) {
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
