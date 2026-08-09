#include "renderer.hpp"

void Renderer::init() {
    m_device->createInstance();
    m_device->pickPhysicalDevice();
    m_device->setupDebugMessenger();

    m_platform->createSurfaceForDevice(m_device);
    m_device->createLogicalDevice();  // features
    m_device->createSwapChain();      // present setting

    m_allocator = createAllocator(m_device);

    loadModels();
    loadShaders();
    initVertexData();
    computeAnimation();
    initIndexData();
    // analyzeMeshes(false);
    optimizeMeshes();
    // shadow don't have LOD
    generateIndexLOD();
    initShadowData();
    initSceneContext();
    initUniformData();

    createCommandBuffers();
    createSyncObjects();
    createSwapchainImageViews();
    createDescriptorSetLayouts();
    createPipelineCache();
    createPipelineLayouts();
    createPipelines();
    createCommandPools();
    createModelImages();
    createSkyboxImage();
    createRenderTargets();
    createSamplers();
    createVertexBuffers();
    createIndexBuffers();
    createInstanceBuffer();
    createUniformBuffers();
    createStorageBuffer();
    createDescriptorPool();
    createDescriptorSets();
}

void Renderer::loadModels() {
    // trace();
    printf("start loading models \n");

    for (auto& [path, model] : m_models) {
        auto res = ParseGltfFile(path.c_str());
        if (res.has_value()) {
            m_models[path] = res.value();
        }
        // for (unsigned int i = 0; i < model.meshes.size(); i++) {
        // 	auto& primitives = model.meshes[i].primitives;
        // 	std::cout << "primitve count: " << primitives.size() << "\n";
        // 	for(auto& attr : primitives[0].attributes) {
        // 		std::cout << "attribute " << attr.first << " - ";
        // 	}
        // 	std::cout << "attribute count: " << primitives[0].attributes.size() << "\n";
        // }

        m_modelMeshTransforms[path].resize(model->meshes_count);
        traverseModelNodesForTransform(path, &model->nodes[0], glm::mat4(1.0f));
    }

    std::cout << "finish loading models \n";
}

void Renderer::initVertexData() {
    {
        cgltf_data* model = m_models[CANDLE_MODEL_PATH];
        m_vertexBuffers.candles.resize(model->meshes_count);
        bool meshHasAnim{false};
        for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
            cgltf_mesh* mesh = &model->meshes[meshIdx];
            auto weights = computeWeights(model, meshIdx);
            // note: if mesh has animation, use each buffers for each attributes
            // otherwise use interleaved attributes to input data to mesh optimizer
            if (!weights.empty()) {
                meshHasAnim = true;
                m_vertexBuffers.candles[meshIdx].resize(4);
            } else {
                meshHasAnim = false;
                m_vertexBuffers.candles[meshIdx].resize(1);
            }

            assert(mesh->primitives_count == 1);
            unsigned int i = 0;
            for (unsigned int i = 0; i < mesh->primitives->attributes_count; i++) {
                // HACK: there's no tangent for animated meshes
                const cgltf_attribute* attribute = &mesh->primitives[0].attributes[i];
                const cgltf_accessor* accessor = attribute->data;

                unsigned char* pBuffer = (unsigned char*)accessor->buffer_view->buffer->data;
                size_t offset = accessor->offset + accessor->buffer_view->offset;
                pBuffer += offset;

                if (meshHasAnim) {
                    // each buffer per attribute data
                    assert(m_vertexBuffers.candles[meshIdx].size() == 4);
                    unsigned int size = accessor->count * accessor->type *
                                        4 /* assume TINYGLTF_COMPONENT_TYPE_FLOAT*/;
                    m_vertexBuffers.candles[meshIdx][i].size = size;
                    m_vertexBuffers.candles[meshIdx][i].needTransfer = true;

                    size_t inputSize = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
                    float* vertexAttribute = (float*)malloc(inputSize * sizeof(float));
                    inputSize = cgltf_accessor_unpack_floats(accessor, vertexAttribute, inputSize);
                    m_vertexBuffers.candles[meshIdx][i].raw = vertexAttribute;
                    i++;
                } else {
                    // one buffer for all attribute interleaved
                    assert(m_vertexBuffers.candles[meshIdx].size() == 1);
                    std::vector<float> src = interleaveAttributes(mesh);
                    unsigned int size = src.size() * 4 /* assume TINYGLTF_COMPONENT_TYPE_FLOAT*/;
                    m_vertexBuffers.candles[meshIdx][0].size = size;
                    m_vertexBuffers.candles[meshIdx][0].needTransfer = true;
                    m_vertexBuffers.candles[meshIdx][0].raw = (void*)malloc(size);
                    memcpy(m_vertexBuffers.candles[meshIdx][0].raw, src.data(), size);

                    break;
                }
            }
        }
    }
}

std::vector<float> Renderer::interleaveAttributes(cgltf_mesh* i_mesh) {
    std::vector<float> res;
    assert(i_mesh->primitives_count == 1);
    cgltf_attribute* attributes = i_mesh->primitives[0].attributes;
    unsigned int count = attributes[0].data->count;
    res.reserve(count * 12);  // 3 for pos, 3 for normal, 4 for tangent, 2 for texCoord

    for (unsigned int vertex_offset = 0; vertex_offset < count; vertex_offset++) {
        const SpvReflectShaderModule* reflection = m_shaderManager.getReflection(CANDLE_MODEL_PATH);
        for (unsigned int i = 0;
             i < reflection->input_variable_count - 1 /*exclude instance buffer*/; i++) {
            std::string reflectAttr = getNameAttrAtIndex(reflection, i);
            cgltf_attribute_type type = getModelAttributeForShaderAttribute(reflectAttr);
            cgltf_accessor* accessor = getAccessorForAttr(i_mesh->primitives[0], type);
            unsigned char* pBuffer = (unsigned char*)accessor->buffer_view->buffer->data;
            size_t offset = accessor->offset + accessor->buffer_view->offset;
            pBuffer += offset;
            float* offset_src = (float*)pBuffer + vertex_offset * accessor->type;
            for (unsigned int o = 0; o < accessor->type; o++) {
                res.push_back(offset_src[o]);
            }
        }
    }

    return res;
}

void Renderer::initIndexData() {
    {
        cgltf_data* model = m_models[SNOWFLAKE_MODEL_PATH];
        cgltf_primitive* primitive = &model->meshes[0].primitives[0];
        cgltf_accessor* accessor = primitive->indices;

        m_indexBuffers.snowflake.raw = getBufferPointerFromAccessor(accessor);
        m_indexBuffers.snowflake.size = accessor->buffer_view->size;
        m_indexBuffers.snowflake.needTransfer = true;
    }

    cgltf_data* model = m_models[CANDLE_MODEL_PATH];
    m_indexBuffers.candles.lod0.resize(model->meshes_count);
    for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
        const auto& mesh = model->meshes[meshIdx];
        assert(mesh.primitives_count == 1);
        const cgltf_accessor* accessor = mesh.primitives[0].indices;

        size_t indexCount = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
        float* indices = (float*)malloc(indexCount * sizeof(float));
        indexCount = cgltf_accessor_unpack_floats(accessor, indices, indexCount);

        m_indexBuffers.candles.lod0[meshIdx].size = indexCount * sizeof(float);
        m_indexBuffers.candles.lod0[meshIdx].needTransfer = true;
        m_indexBuffers.candles.lod0[meshIdx].raw = indices;
    }
}

void Renderer::computeAnimation() {
    cgltf_data* model = m_models[CANDLE_MODEL_PATH];

    for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
        auto weights = computeWeights(model, meshIdx);
        // check if there is animation from gltf sampler side
        if (!weights.empty()) {
            computeMorphTargets(model, meshIdx, weights);
        }
    }
}

std::vector<float> Renderer::computeWeights(cgltf_data* i_model, unsigned int meshIdx) {
    ZoneScopedN("ComputeAnimation - weight");
    // sample animation
    assert(i_model->animations_count == 1);
    cgltf_animation* anims = &i_model->animations[0];
    cgltf_animation_channel* channels = anims->channels;
    unsigned int channelIdx = 0;
    for (; channelIdx < anims->channels_count; channelIdx++) {
        cgltf_node* node = channels[channelIdx].target_node;
        if (cgltf_node_index(i_model, node) == meshIdx) break;
    }

    if (channelIdx == i_model->animations_count) return {};

    cgltf_animation_sampler* sampler = channels[channelIdx].sampler;
    const cgltf_accessor* inputAcc = sampler->input;
    size_t inputSize = cgltf_accessor_unpack_floats(inputAcc, nullptr, 0);
    float* samplerInput = (float*)malloc(inputSize * sizeof(float));
    inputSize = cgltf_accessor_unpack_floats(inputAcc, samplerInput, inputSize);

    m_currentAnimTime += m_currentDeltaTime * CANDLE_ANIMATION_SPEED;
    if (m_currentAnimTime > inputAcc->max[0]) m_currentAnimTime -= inputAcc->max[0];

    unsigned int hi = 1;
    for (; hi < inputAcc->count; hi++) {
        if (samplerInput[hi] > m_currentAnimTime) break;
    }

    float ratio =
        (m_currentAnimTime - samplerInput[hi - 1]) / (samplerInput[hi] - samplerInput[hi - 1]);

    const cgltf_accessor* outputAcc = sampler->output;
    float* samplerOutput = (float*)getBufferPointerFromAccessor(outputAcc);

    // Warning: is multi-ply with inputAcc->count correct? shouldn't it be
    // cgltf_primitive->targets_count ?
    const float* liWeights = samplerOutput + (hi - 1) * inputAcc->count;
    const float* hiWeights = samplerOutput + hi * inputAcc->count;

    std::vector<float> res{};
    res.resize(inputAcc->count);
    for (unsigned int i = 0; i < res.size(); i++) {
        res[i] = hiWeights[i] * ratio + liWeights[i] * (1 - ratio);
        // std::cout << "hiWeights[" << i << "]" << " = " << hiWeights[i] << "\n";
        // std::cout << "liWeights[" << i << "]" << " = " << liWeights[i] << "\n";
        // std::cout << "res[" << i << "]" << " = " << res[i] << "\n";
    }

    return res;
}

void Renderer::computeMorphTargets(cgltf_data* i_model, unsigned int i_meshIdx,
                                   std::vector<float> i_weights) {
    ZoneScopedN("ComputeAnimation - morph target");
    const cgltf_mesh* mesh = &i_model->meshes[i_meshIdx];
    // re-set to original position
    assert(mesh->primitives_count == 1);
    cgltf_attribute* attributes = mesh->primitives[0].attributes;
    cgltf_accessor* posAccessor =
        getAccessorForAttr(mesh->primitives[0], cgltf_attribute_type_position);
    size_t floatCount = cgltf_accessor_unpack_floats(posAccessor, NULL, 0);
    float* position = (float*)malloc(floatCount * sizeof(float));
    floatCount = cgltf_accessor_unpack_floats(posAccessor, position, floatCount);

    // NOTE: Position is NOT at the first attribute
    int posBufferIdx = getIndexForAttr(mesh->primitives[0], cgltf_attribute_type_position);
    assert(posBufferIdx != -1);

    m_vertexBuffers.candles[i_meshIdx][posBufferIdx].needTransfer = true;
    m_vertexBuffers.candles[i_meshIdx][posBufferIdx].size = posAccessor->count * sizeof(glm::vec3);
    assert(posAccessor->count * sizeof(glm::vec3) == floatCount * sizeof(float));
    memcpy(m_vertexBuffers.candles[i_meshIdx][posBufferIdx].raw, position,
           posAccessor->count * sizeof(glm::vec3));
    glm::vec3* pPosVec =
        reinterpret_cast<glm::vec3*>(m_vertexBuffers.candles[i_meshIdx][posBufferIdx].raw);

    // accumulate with each morph target
    cgltf_morph_target* morphTargets = mesh->primitives[0].targets;
    int targetsCount = mesh->primitives[0].targets_count;
    assert(i_weights.size() == targetsCount);
    for (unsigned int morphIdx = 0; morphIdx < targetsCount; morphIdx++) {
        cgltf_accessor* morphAccessor =
            getAccessorForAttr(morphTargets[morphIdx], cgltf_attribute_type_position);
        assert(posAccessor->count == morphAccessor->count);
        unsigned char* pBuffer = (unsigned char*)posAccessor->buffer_view->buffer->data;
        size_t offset = posAccessor->offset + posAccessor->buffer_view->offset;
        const unsigned char* pMorphData = pBuffer + offset;
        const glm::vec3* pMorphVec = reinterpret_cast<const glm::vec3*>(pMorphData);

        for (unsigned int vertexIdx = 0; vertexIdx < morphAccessor->count; vertexIdx++) {
            pPosVec[vertexIdx] += pMorphVec[vertexIdx] * i_weights[morphIdx];
        }
    }
}

void Renderer::optimizeMeshes() {
    cgltf_data* model = m_models[CANDLE_MODEL_PATH];
    assert(m_vertexBuffers.candles.size() == model->meshes_count);
    m_indexBuffers.candles.lod0.resize(m_vertexBuffers.candles.size());
    for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
        // only generate LOD for mesh don't have animation (didn't interleave data)
        if (m_vertexBuffers.candles[meshIdx].size() != 1) continue;

        cgltf_primitive& primitive = model->meshes[meshIdx].primitives[0];
        cgltf_accessor* indexAccessor = primitive.indices;
        cgltf_accessor* posAccessor = getAccessorForAttr(primitive, cgltf_attribute_type_position);

        unsigned int* indices = (unsigned int*)m_indexBuffers.candles.lod0[meshIdx].raw;
        const float* vertex = (float*)m_vertexBuffers.candles[meshIdx][0].raw;

        unsigned int* tempIndices =
            (unsigned int*)malloc(sizeof(unsigned int) * indexAccessor->count);
        meshopt_optimizeVertexCache(tempIndices, indices, indexAccessor->count, posAccessor->count);
        meshopt_optimizeOverdraw(indices, tempIndices, indexAccessor->count, vertex,
                                 posAccessor->count, 48, c_overdrawThreshold);

        unsigned int* tempVertices =
            (unsigned int*)malloc(m_vertexBuffers.candles[meshIdx][0].size);
        unsigned int newVertexSize = meshopt_optimizeVertexFetch(
            tempVertices, indices, indexAccessor->count, vertex, posAccessor->count, 48);

        free(m_vertexBuffers.candles[meshIdx][0].raw);
        unsigned int newSize = newVertexSize * 12 * sizeof(float);
        m_vertexBuffers.candles[meshIdx][0].raw = realloc(tempVertices, newSize);
        m_vertexBuffers.candles[meshIdx][0].size = newSize;
        m_vertexBuffers.candles[meshIdx][0].needTransfer = true;
        m_indexBuffers.candles.lod0[meshIdx].needTransfer = true;

        free(tempIndices);
    }
}

void Renderer::generateIndexLOD() {
    m_indexBuffers.candles.lod1.resize(m_vertexBuffers.candles.size());
    cgltf_data* model = m_models[CANDLE_MODEL_PATH];
    for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
        // only generate LOD for mesh don't have animation (only 1 interleaved buffer data)
        if (m_vertexBuffers.candles[meshIdx].size() != 1) continue;

        const unsigned int* indices = (unsigned int*)m_indexBuffers.candles.lod0[meshIdx].raw;
        unsigned int indexSize = m_indexBuffers.candles.lod0[meshIdx].size;
        const float* vertex = (float*)m_vertexBuffers.candles[meshIdx][0].raw;
        unsigned int vertexCount = m_vertexBuffers.candles[meshIdx][0].size / (12 * sizeof(float));

        unsigned int* des = (unsigned int*)malloc(indexSize);
        float* resultErr{};

        size_t newIdxSize = meshopt_simplifyWithAttributes(
            des, indices, indexSize / sizeof(unsigned int), vertex, vertexCount, 48, vertex + 3, 48,
            s_attrWeights, 9, nullptr, 0, s_targetError, 0, resultErr);

        assert(newIdxSize <= indexSize);
        if (m_indexBuffers.candles.lod1[meshIdx].raw != nullptr) {
            free(m_indexBuffers.candles.lod1[meshIdx].raw);
        }
        m_indexBuffers.candles.lod1[meshIdx].raw =
            (unsigned int*)realloc(des, newIdxSize * sizeof(unsigned int));
        m_indexBuffers.candles.lod1[meshIdx].size = newIdxSize * sizeof(unsigned int);
        m_indexBuffers.candles.lod1[meshIdx].needTransfer = true;
    }
}

void Renderer::initUniformData() {
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_graphicUniformBuffers.shadow.perMeshTransform[i].size =
            sizeof(ShadowPerMeshTransform) * CANDLES_BASE_MESH_COUNT;
        m_graphicUniformBuffers.shadow.lightTransform.size = sizeof(ShadowLightingTransform);
    }

    m_graphicUniformBuffers.shadow.perInstanceTransform.size =
        sizeof(glm::mat4) * CANDLES_INSTANCE_MAX;
}

void Renderer::initShadowData() {
    cgltf_data* model = m_models[CANDLE_MODEL_PATH];
    std::vector<float> shadowVertices;
    std::vector<unsigned int> shadowIndices;
    float shadowMeshIdx = 0;
    // candles meshes
    for (unsigned int meshIdx = 0; meshIdx < model->meshes_count; meshIdx++) {
        // only the base of the candles cast shadow
        if (m_vertexBuffers.candles[meshIdx].size() == 1) {
            const unsigned int* i =
                reinterpret_cast<unsigned int*>(m_indexBuffers.candles.lod0[meshIdx].raw);
            const unsigned int iCount =
                m_indexBuffers.candles.lod0[meshIdx].size / sizeof(unsigned int);
            const unsigned int vertexOffset = shadowVertices.size() / 4;
            for (unsigned int idx = 0; idx < iCount; idx++) {
                shadowIndices.push_back(i[idx] + vertexOffset);
            }

            const float* v = reinterpret_cast<float*>(m_vertexBuffers.candles[meshIdx][0].raw);
            const unsigned int vCount = m_vertexBuffers.candles[meshIdx][0].size / sizeof(float);
            const unsigned int stride = 12;
            // interleaved vertex data already in shader attr order
            const unsigned int offset = 0;
            for (unsigned int idx = 0 + offset; idx < vCount; idx += stride) {
                for (unsigned int vt = 0; vt < 3; vt++) {
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

void Renderer::initSceneContext() {
    ZoneScopedN("Update Graphic Transform Uniform Buffer");

    // view
    m_sceneContext.camView = m_camera.getViewMatrix();
    vk::Extent2D swapChainExtent = m_device->getSwapchainExtent();
    m_sceneContext.camProjection =
        glm::perspective(m_camera.getZoom(), swapChainExtent.width / (float)swapChainExtent.height,
                         s_nearPlane, s_farPlane);
    m_sceneContext.lightView =
        glm::lookAt(s_lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_sceneContext.lightProjection =
        glm::ortho(s_shadowLeftPlane, s_shadowRightPlane, s_shadowBotPlane, s_shadowTopPlane,
                   s_nearPlane, s_shadowFarPlane);

    // snowflake
    {
        glm::mat4& model = m_sceneContext.snowflakeModel;
        model = glm::mat4(1.0f);
        model = glm::translate(
            model, glm::vec3(s_snowTranslate[0], s_snowTranslate[1], s_snowTranslate[2]));
        if (s_snowRotate[0] != 0.f || s_snowRotate[1] != 0.f || s_snowRotate[2] != 0.f)
            model = glm::rotate(model, m_lastTime * glm::radians(90.0f),
                                glm::vec3(s_snowRotate[0], s_snowRotate[1], s_snowRotate[2]));
        model = glm::scale(model, glm::vec3(s_snowScale[0], s_snowScale[1], s_snowScale[2]));
    }

    {
        // candles
        cgltf_data* model = m_models[CANDLE_MODEL_PATH];
        {
            unsigned int meshCount = model->meshes_count;
            glm::mat4& candlesModel = m_sceneContext.candlesModel;
            candlesModel = glm::mat4(1.0f);
            candlesModel = glm::translate(
                candlesModel,
                glm::vec3(s_candlesTranslate[0], s_candlesTranslate[1], s_candlesTranslate[2]));
            candlesModel = glm::scale(
                candlesModel, glm::vec3(s_candlesScale[0], s_candlesScale[1], s_candlesScale[2]));

            for (unsigned int meshIdx = 0; meshIdx < meshCount; meshIdx++) {
                // mesh local transform
                glm::mat4 localMeshModel =
                    candlesModel * m_modelMeshTransforms[CANDLE_MODEL_PATH][meshIdx];
                m_sceneContext.candlesMeshesModels.push_back(localMeshModel);

                // if mesh cast shadow
                if (m_vertexBuffers.candles[meshIdx].size() == 1) {
                    m_sceneContext.shadowBatchedModels.push_back(localMeshModel);
                }
            }
        }
    }

    {
        // floor shadow
        glm::mat4& model{m_sceneContext.floorModel};
        model = glm::translate(model, glm::vec3(0.f, -0.1f, 0.f));
        model = glm::rotate(model, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        model = glm::scale(model, glm::vec3(15.f, 15.f, 15.f));
    }

    {
        for (unsigned int i = 0; i < m_towerInstanceRaw.size(); i++) {
            glm::mat4 instanceModel;
            instanceModel[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            instanceModel[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            instanceModel[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            instanceModel[3] = glm::vec4(m_towerInstanceRaw[i].pos.x, m_towerInstanceRaw[i].pos.y,
                                         m_towerInstanceRaw[i].pos.z, 1.0f);

            m_sceneContext.shadowInstanceModels.push_back(instanceModel);
        }
    }
}

cgltf_accessor* Renderer::getAccessorForAttr(cgltf_primitive& i_primitive,
                                             cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i_primitive.attributes[i].data;
        }
    }

    return nullptr;
}

cgltf_accessor* Renderer::getAccessorForAttr(cgltf_morph_target& i_primitive,
                                             cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i_primitive.attributes[i].data;
        }
    }

    return nullptr;
}

int Renderer::getIndexForAttr(cgltf_primitive& i_primitive, cgltf_attribute_type i_type) {
    for (unsigned int i = 0; i < i_primitive.attributes_count; i++) {
        if (i_primitive.attributes[i].type == i_type) {
            return i;
        }
    }

    return -1;
}

cgltf_attribute_type Renderer::getModelAttributeForShaderAttribute(std::string i_attributeName) {
    if (i_attributeName == "a_position") return cgltf_attribute_type_position;
    if (i_attributeName == "a_normal") return cgltf_attribute_type_normal;
    if (i_attributeName == "a_tangent") return cgltf_attribute_type_tangent;
    if (i_attributeName == "a_texCoord") return cgltf_attribute_type_texcoord;

    return cgltf_attribute_type_invalid;
}

std::string Renderer::getNameAttrAtIndex(const SpvReflectShaderModule* module, uint8_t idx) {
    for (unsigned int i = 0; i < module->input_variable_count; i++) {
        if (module->input_variables[i]->location == idx) return module->input_variables[i]->name;
    }
    return "";
}

unsigned char* Renderer::getBufferPointerFromAccessor(const cgltf_accessor* i_accessor) {
    unsigned char* pBuffer = (unsigned char*)i_accessor->buffer_view->buffer->data;
    size_t offset = i_accessor->offset + i_accessor->buffer_view->offset;
    return pBuffer + offset;
}

float* Renderer::allocateFloatBufferForAccessor(const cgltf_accessor* i_accessor) {
    size_t count = cgltf_accessor_unpack_floats(i_accessor, nullptr, 0);
    float* buffer = (float*)malloc(count * sizeof(float));
    count = cgltf_accessor_unpack_floats(i_accessor, buffer, count);
    assert(i_accessor->buffer_view->size == count * sizeof(float));
    return buffer;
}

void Renderer::createSwapchainImageViews() {
    m_swapChainImageViews.resize(m_device->m_swapChainImages.size());

    for (uint32_t i = 0; i < m_device->m_swapChainImages.size(); i++) {
        m_swapChainImageViews[i] = m_device->createImageView(m_device->m_swapChainImages[i],
                                                             m_device->m_swapchainImageFormat,
                                                             vk::ImageAspectFlagBits::eColor, 1);
    }
}

void Renderer::createVertexBuffers() {
    // Snowflake
    {
        Buffer& snowBuffer = m_vertexBuffers.snowflake;

        if (snowBuffer.needTransfer) {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAlloc{};
            m_allocator.createBuffer(
                snowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAlloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
            memcpy(data, snowBuffer.raw, snowBuffer.size);
            vmaUnmapMemory(m_allocator, stagingBufferAlloc);
            m_allocator.createBuffer(
                snowBuffer.size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, snowBuffer.buffer, snowBuffer.allocation);
            copyBuffer(stagingBuffer, snowBuffer.buffer, snowBuffer.size);
            snowBuffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAlloc);
        }
    }

    // Candles
    {
        Object objIdx = Object::CANDLE;
        tinygltf::Model& model = m_model[objIdx];

        for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
            assert(model.meshes.size() == m_vertexBuffers.candles.size());
            for (unsigned int attrIdx = 0; attrIdx < m_vertexBuffers.candles[meshIdx].size();
                 attrIdx++) {
                if (m_vertexBuffers.candles[meshIdx][attrIdx].needTransfer == false ||
                    m_vertexBuffers.candles[meshIdx][attrIdx].size == 0)
                    continue;

                // Transfer vertex position animation data
                VkBuffer stagingBuffer;
                VmaAllocation stagingAlloc;
                unsigned int size = m_vertexBuffers.candles[meshIdx][attrIdx].size;

                m_allocator.createBuffer(
                    size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingAlloc);

                void* data;
                vmaMapMemory(m_allocator, stagingAlloc, &data);
                memcpy(data, m_vertexBuffers.candles[meshIdx][attrIdx].raw,
                       static_cast<size_t>(size));
                vmaUnmapMemory(m_allocator, stagingAlloc);

                m_allocator.createBuffer(
                    size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_vertexBuffers.candles[meshIdx][attrIdx].buffer,
                    m_vertexBuffers.candles[meshIdx][attrIdx].allocation);

                VkBufferCopy copyRegion{};
                copyRegion.size = size;

                copyBuffer(stagingBuffer, m_vertexBuffers.candles[meshIdx][attrIdx].buffer, size);

                vkDestroyBuffer(device, stagingBuffer, nullptr);
                vmaFreeMemory(m_allocator, stagingAlloc);

                m_vertexBuffers.candles[meshIdx][attrIdx].needTransfer = false;
            }
        }
    }

    // Shadow
    {
        Buffer& shadowBuffer = m_vertexBuffers.shadow;
        if (shadowBuffer.needTransfer) {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAlloc{};
            m_allocator.createBuffer(
                shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAlloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
            memcpy(data, shadowBuffer.raw, shadowBuffer.size);
            vmaUnmapMemory(m_allocator, stagingBufferAlloc);
            m_allocator.createBuffer(
                shadowBuffer.size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowBuffer.buffer, shadowBuffer.allocation);
            copyBuffer(stagingBuffer, shadowBuffer.buffer, shadowBuffer.size);
            shadowBuffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAlloc);
        }
    }

    // Quad
    {
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAlloc{};

        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAlloc{};

        int size = sizeof(quadListVertices);
        m_allocator.createBuffer(
            size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferAlloc);
        void* data;
        vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
        memcpy(data, quadListVertices, size);
        vmaUnmapMemory(m_allocator, stagingBufferAlloc);
        m_allocator.createBuffer(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferAlloc);
        copyBuffer(stagingBuffer, vertexBuffer, size);

        m_vertexBuffers.quad.buffer = vertexBuffer;
        m_vertexBuffers.quad.allocation = vertexBufferAlloc;

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vmaFreeMemory(m_allocator, stagingBufferAlloc);
    }

    // Cube
    {
        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAlloc{};

        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAlloc{};

        int size = sizeof(skyboxVertices);
        m_allocator.createBuffer(
            size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferAlloc);
        void* data;
        vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
        memcpy(data, skyboxVertices, size);
        vmaUnmapMemory(m_allocator, stagingBufferAlloc);
        m_allocator.createBuffer(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferAlloc);
        copyBuffer(stagingBuffer, vertexBuffer, size);

        m_vertexBuffers.cube.buffer = vertexBuffer;
        m_vertexBuffers.cube.allocation = vertexBufferAlloc;

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vmaFreeMemory(m_allocator, stagingBufferAlloc);
    }
}

void Renderer::createIndexBuffers() {
    {
        // Snowflake
        Buffer& snowIdxBuffer = m_indexBuffers.snowflake;

        if (snowIdxBuffer.needTransfer) {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAloc{};

            m_allocator.createBuffer(
                snowIdxBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAloc, &data);
            memcpy(data, snowIdxBuffer.raw, snowIdxBuffer.size);
            vmaUnmapMemory(m_allocator, stagingBufferAloc);
            m_allocator.createBuffer(
                snowIdxBuffer.size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, snowIdxBuffer.buffer,
                snowIdxBuffer.allocation);
            copyBuffer(stagingBuffer, snowIdxBuffer.buffer, snowIdxBuffer.size);
            snowIdxBuffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAloc);
        }
    }

    // candles lod0
    {
        for (auto& buffer : m_indexBuffers.candles.lod0) {
            if (buffer.needTransfer == false || buffer.size == 0) continue;

            Buffer newBuffer{};
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAloc{};

            m_allocator.createBuffer(
                buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAloc, &data);
            memcpy(data, buffer.raw, buffer.size);
            vmaUnmapMemory(m_allocator, stagingBufferAloc);
            m_allocator.createBuffer(
                buffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.buffer, buffer.allocation);
            copyBuffer(stagingBuffer, buffer.buffer, buffer.size);

            buffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAloc);
        }
    }

    // candles lod1, transfer meshopt generated data
    // raw lod data already setup in generateIndexLOD func
    {
        for (unsigned int i = 0; i < m_indexBuffers.candles.lod1.size(); i++) {
            auto& indexBuffer = m_indexBuffers.candles.lod1[i];
            if (indexBuffer.needTransfer == false || indexBuffer.size == 0) continue;

            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAloc{};
            // same size with LOD0 we need the biggest size possible for LOD1
            // for the need of re-allocating with different size
            uint32_t size{m_indexBuffers.candles.lod0[i].size};

            m_allocator.createBuffer(
                size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAloc, &data);
            memcpy(data, indexBuffer.raw, size);
            vmaUnmapMemory(m_allocator, stagingBufferAloc);
            m_allocator.createBuffer(
                size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer.buffer, indexBuffer.allocation);
            copyBuffer(stagingBuffer, indexBuffer.buffer, size);

            indexBuffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAloc);
        }
    }

    {
        // Shadow
        Buffer& shadowBuffer = m_indexBuffers.shadow;
        if (shadowBuffer.needTransfer) {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAlloc{};
            m_allocator.createBuffer(
                shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferAlloc);
            void* data;
            vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
            memcpy(data, shadowBuffer.raw, shadowBuffer.size);
            vmaUnmapMemory(m_allocator, stagingBufferAlloc);
            m_allocator.createBuffer(
                shadowBuffer.size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowBuffer.buffer, shadowBuffer.allocation);
            copyBuffer(stagingBuffer, shadowBuffer.buffer, shadowBuffer.size);
            shadowBuffer.needTransfer = false;

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vmaFreeMemory(m_allocator, stagingBufferAlloc);
        }
    }
}

void Renderer::createUniformBuffers() {
    createGraphicUniformBuffers();
    createComputeUniformBuffers();
}

void Renderer::createGraphicUniformBuffers() {
    // snowflake
    {
        VkDeviceSize bufferSize = sizeof(SnowTransform);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_allocator.createBuffer(
                bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_graphicUniformBuffers.snowflake[i].buffer,
                m_graphicUniformBuffers.snowflake[i].allocation);

            vmaMapMemory(m_allocator, m_graphicUniformBuffers.snowflake[i].allocation,
                         &m_graphicUniformBuffers.snowflake[i].raw);
        }
    }

    // candles
    {
        Object objIdx = Object::CANDLE;
        tinygltf::Model& model = m_model[objIdx];

        // transform uniform
        {
            unsigned int meshCount = model.meshes.size();
            VkDeviceSize bufferSize = sizeof(CandlesPerMeshTransform) * meshCount;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                m_allocator.createBuffer(
                    bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    m_graphicUniformBuffers.candles.perMeshTransform[i].buffer,
                    m_graphicUniformBuffers.candles.perMeshTransform[i].allocation);

                vmaMapMemory(m_allocator,
                             m_graphicUniformBuffers.candles.perMeshTransform[i].allocation,
                             &m_graphicUniformBuffers.candles.perMeshTransform[i].raw);
            }
        }

        // lighting uniform
        {
            VkDeviceSize bufferSize = sizeof(CandlesLightingTransform);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                m_allocator.createBuffer(
                    bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    m_graphicUniformBuffers.candles.lightingTransform[i].buffer,
                    m_graphicUniformBuffers.candles.lightingTransform[i].allocation);

                vmaMapMemory(m_allocator,
                             m_graphicUniformBuffers.candles.lightingTransform[i].allocation,
                             &m_graphicUniformBuffers.candles.lightingTransform[i].raw);
            }
        }
    }

    // floor
    {
        VkDeviceSize bufferSize = sizeof(FloorTransform);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_allocator.createBuffer(
                bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_graphicUniformBuffers.floor[i].buffer,
                m_graphicUniformBuffers.floor[i].allocation);

            vmaMapMemory(m_allocator, m_graphicUniformBuffers.floor[i].allocation,
                         &m_graphicUniformBuffers.floor[i].raw);
        }
    }

    // skybox
    {
        VkDeviceSize bufferSize = sizeof(SkyboxTransform);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_allocator.createBuffer(
                bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_graphicUniformBuffers.skybox[i].buffer,
                m_graphicUniformBuffers.skybox[i].allocation);

            vmaMapMemory(m_allocator, m_graphicUniformBuffers.skybox[i].allocation,
                         &m_graphicUniformBuffers.skybox[i].raw);
        }
    }

    // shadow
    {
        // heuristic mesh casting shadow size
        VkDeviceSize meshBufferCap = m_graphicUniformBuffers.shadow.perMeshTransform[0].size;
        assert(meshBufferCap > 0);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_allocator.createBuffer(
                meshBufferCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_graphicUniformBuffers.shadow.perMeshTransform[i].buffer,
                m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation);

            vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation,
                         &m_graphicUniformBuffers.shadow.perMeshTransform[i].raw);
        }

        VkDeviceSize transCap = m_graphicUniformBuffers.shadow.lightTransform.size;
        assert(transCap > 0);
        m_allocator.createBuffer(
            transCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_graphicUniformBuffers.shadow.lightTransform.buffer,
            m_graphicUniformBuffers.shadow.lightTransform.allocation);

        vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.lightTransform.allocation,
                     &m_graphicUniformBuffers.shadow.lightTransform.raw);

        VkDeviceSize perInstanceCap = m_graphicUniformBuffers.shadow.perInstanceTransform.size;
        assert(perInstanceCap > 0);
        m_allocator.createBuffer(
            perInstanceCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_graphicUniformBuffers.shadow.perInstanceTransform.buffer,
            m_graphicUniformBuffers.shadow.perInstanceTransform.allocation);

        vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.perInstanceTransform.allocation,
                     &m_graphicUniformBuffers.shadow.perInstanceTransform.raw);
    }
}

void Renderer::createComputeUniformBuffers() {
    m_computeUniformBuffers.snowflake.vortex[0].raw =
        static_cast<void*>(new Vortex[MAX_VORTEX_COUNT]);
    m_computeUniformBuffers.snowflake.vortex[1].raw =
        static_cast<void*>(new Vortex[MAX_VORTEX_COUNT]);

    VkDeviceSize bufferSize = sizeof(Vortex) * MAX_VORTEX_COUNT;
    m_allocator.createBuffer(
        bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_computeUniformBuffers.snowflake.vortex[0].buffer,
        m_computeUniformBuffers.snowflake.vortex[0].allocation);
    vmaMapMemory(m_allocator, m_computeUniformBuffers.snowflake.vortex[0].allocation,
                 &m_computeUniformBuffers.snowflake.vortex[0].raw);

    m_allocator.createBuffer(
        bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_computeUniformBuffers.snowflake.vortex[1].buffer,
        m_computeUniformBuffers.snowflake.vortex[1].allocation);
    vmaMapMemory(m_allocator, m_computeUniformBuffers.snowflake.vortex[1].allocation,
                 &m_computeUniformBuffers.snowflake.vortex[1].raw);

    for (unsigned int i = 0; i < MAX_VORTEX_COUNT; i++) {
        Vortex& vortex0 = ((Vortex*)m_computeUniformBuffers.snowflake.vortex[0].raw)[i];
        Vortex& vortex1 = ((Vortex*)m_computeUniformBuffers.snowflake.vortex[1].raw)[i];
        vortex0.pos.x = vortex1.pos.x =
            generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
        vortex0.pos.y = vortex1.pos.y =
            generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
        vortex0.pos.z = vortex1.pos.z =
            generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
        vortex0.height = vortex1.height = generateRandomFloat(5.f, 10.f);

        s_basePhase[i] = generateRandomFloat(0.f, PHASE_RANGE);
        s_baseForce[i] = generateRandomFloat(MIN_FORCE, MAX_FORCE);
        s_baseRadius[i] = generateRandomFloat(MIN_RADIUS, MAX_RADIUS);
        vortex0.force = vortex1.force = s_baseForce[i];
        vortex0.radius = vortex1.radius = s_baseRadius[i];
    }
}
