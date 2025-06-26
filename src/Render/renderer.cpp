#include "renderer.hpp"

	void initTracy(){
		tracyContext = TracyVkContextCalibrated(instance, physicalDevice, m_device, m_graphicQueue, tracyCommandBuffer, vkGetInstanceProcAddr, vkGetDeviceProcAddr);
		
		// VkQueryPoolCreateInfo poolInfo;
		// poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO; 
		// poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		// poolInfo.queryCount = 1024 * 8;
		// poolInfo.pNext = nullptr;
		// if(vkCreateQueryPool(m_device, &poolInfo, nullptr, &timestampPool) != VK_SUCCESS)
		// {
        //     throw std::runtime_error("failed to create query pool!");
		// }
	}

	void initImGui(){
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		
		ImGui_ImplGlfw_InitForVulkan(window, true);

		std::array<VkDescriptorPoolSize, 1> poolSize;
		poolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize[0].descriptorCount = 10;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 10;
		poolInfo.pPoolSizes = poolSize.data();
		poolInfo.poolSizeCount = poolSize.size();

        if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

		ImGui_ImplVulkan_InitInfo info{};
		info.Instance = instance;
		info.PhysicalDevice = physicalDevice;
		info.Device = m_device;
		info.QueueFamily = findQueueFamilies(physicalDevice).graphicFamily.value();
		info.Queue = m_graphicQueue;
		info.DescriptorPool = imguiDescriptorPool;
		info.MinImageCount = m_swapChainImages.size();
		info.ImageCount = m_swapChainImages.size();
		info.RenderPass = m_renderPasses.combine;
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; 
		
		// info.CheckVkResultFn = CheckImGuiResult;
		ImGui_ImplVulkan_Init(&info);
	}

    void initVulkan() {
        createSwapchainImageViews();
        createRenderPasses();
        createDescriptorSetLayouts();
		createPipelineCache();
		createPipelineLayouts();
		createPipelines();
        createCommandPools();
        createModelImages();
		createSkyboxImage();
        createRenderTargets();
        createFramebuffers();
        createSamplers();
		loadInstanceData();
        createVertexBuffers();
        createIndexBuffers();
		createInstanceBuffer();
		createUniformBuffers();
		createStorageBuffer();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();

    }


    void updateGraphicUniformBuffer() {
		ZoneScopedN("Update Graphic Transform Uniform Buffer");

		// floor base pass
		FloorTransform* floorTrans = (FloorTransform*)m_graphicUniformBuffers.floor[m_currentFrame].raw;

		std::vector<ShadowPerMeshTransform> shadowMeshUniform;
		// snowflake
		{
			SnowTransform ubo{};
			ubo.model = glm::mat4(1.0f);
			ubo.model = glm::translate(ubo.model, glm::vec3(s_snowTranslate[0], s_snowTranslate[1], s_snowTranslate[2]));
			if(s_snowRotate[0] != 0.f || s_snowRotate[1] != 0.f || s_snowRotate[2] != 0.f)
				ubo.model = glm::rotate(ubo.model, m_lastTime * glm::radians(90.0f), glm::vec3(s_snowRotate[0], s_snowRotate[1], s_snowRotate[2]));
			ubo.model = glm::scale(ubo.model, glm::vec3(s_snowScale[0], s_snowScale[1], s_snowScale[2]));
			glm::mat4 view = g_camera.getViewMatrix();
			glm::mat4 proj = glm::perspective(g_camera.getZoom(), swapChainExtent.width / (float) swapChainExtent.height, s_nearPlane, s_farPlane);
			proj[1][1] *= -1;

			ubo.viewProj = proj * view;

			*(SnowTransform*)m_graphicUniformBuffers.snowflake[m_currentFrame].raw = ubo;
		}

		{
			// candles
			Object objIdx = Object::CANDLE;
			tinygltf::Model& model = m_model[objIdx];
			{
				unsigned int meshCount = model.meshes.size();
				CandlesPerMeshTransform candleMeshModel{};
				candleMeshModel.model = glm::mat4(1.0f);
				candleMeshModel.model = glm::translate(candleMeshModel.model, glm::vec3(s_candlesTranslate[0], s_candlesTranslate[1], s_candlesTranslate[2]));
				candleMeshModel.model = glm::scale(candleMeshModel.model, glm::vec3(s_candlesScale[0], s_candlesScale[1], s_candlesScale[2]));

				for (unsigned int meshIdx = 0; meshIdx < meshCount; meshIdx++){
					// mesh local transform
					glm::mat4 localMeshModel = candleMeshModel.model * m_modelMeshTransforms[objIdx][meshIdx];
					CandlesPerMeshTransform* uniformMapped = (CandlesPerMeshTransform*)m_graphicUniformBuffers.candles.perMeshTransform[m_currentFrame].raw;
					uniformMapped[meshIdx].model = localMeshModel;

					// if mesh cast shadow
					if (m_vertexBuffers.candles[meshIdx].size() == 1) {
						ShadowPerMeshTransform model{localMeshModel};
						shadowMeshUniform.push_back(model);
					}
				}

				CandlesLightingTransform* candlesUBO = (CandlesLightingTransform*)m_graphicUniformBuffers.candles.lightingTransform[m_currentFrame].raw;
				glm::mat4 view = g_camera.getViewMatrix();
				glm::mat4 proj = glm::perspective(g_camera.getZoom(), swapChainExtent.width / (float) swapChainExtent.height, s_nearPlane, s_farPlane);
				proj[1][1] *= -1;

				candlesUBO->viewProj = proj * view;
				candlesUBO->lightPos = s_lightDir;
				candlesUBO->camPos = g_camera.getPostion();

				floorTrans->camViewProj = candlesUBO->viewProj; 
				floorTrans->lightPos = candlesUBO->lightPos; 
				floorTrans->camPos = candlesUBO->camPos; 

				SkyboxTransform* skyboxUBO = (SkyboxTransform*)m_graphicUniformBuffers.skybox[m_currentFrame].raw;
				// remove translation part
				skyboxUBO->camView = glm::mat4(glm::mat3(view));
				skyboxUBO->camProj = proj;
			}

			// floor shadow
			ShadowPerMeshTransform floor{glm::mat4(1.0f)};
			floor.model = glm::translate(floor.model, glm::vec3(0.f, -0.001f, 0.f));
			floor.model = glm::rotate(floor.model, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
			floor.model = glm::scale(floor.model, glm::vec3(15.f, 15.f, 15.f));

			floorTrans->model = floor.model; 
		}

		{
			glm::mat4 view = glm::lookAt(s_lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			//// note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
			//lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); 
			glm::mat4 proj = glm::ortho(s_shadowLeftPlane, s_shadowRightPlane, s_shadowBotPlane, s_shadowTopPlane, s_nearPlane, s_shadowFarPlane);
			proj[1][1] *= -1;
			ShadowLightingTransform* shadow = (ShadowLightingTransform*)m_graphicUniformBuffers.shadow.lightTransform.raw;
			shadow->viewProj = proj * view; 
			floorTrans->lightViewProj = shadow->viewProj;

			assert(m_sceneContext.candlesShadowMeshCount == shadowMeshUniform.size());
			unsigned int shadowUniformSize = shadowMeshUniform.size() * sizeof(ShadowPerMeshTransform);
			memcpy(m_graphicUniformBuffers.shadow.perMeshTransform[m_currentFrame].raw, shadowMeshUniform.data(), shadowUniformSize);
			m_graphicUniformBuffers.shadow.perMeshTransform[m_currentFrame].size = shadowUniformSize;

			unsigned int shadowInstanceSize = sizeof(glm::mat4) * m_towerInstanceRaw.size();
			m_graphicUniformBuffers.shadow.perInstanceTransform.size = shadowInstanceSize;
			glm::mat4* pShadow = (glm::mat4*)m_graphicUniformBuffers.shadow.perInstanceTransform.raw;
			for (unsigned int i = 0; i < m_towerInstanceRaw.size(); i++) {
				glm::mat4 instanceModel;
				instanceModel[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
				instanceModel[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
				instanceModel[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
				instanceModel[3] = glm::vec4(m_towerInstanceRaw[i].pos.x, m_towerInstanceRaw[i].pos.y, m_towerInstanceRaw[i].pos.z, 1.0f);

				pShadow[i] = instanceModel;
			}
		}
    }
	
	void updateComputeUniformBuffer() {
		ZoneScopedN("Update Compute Vortex Uniform Buffer");
		for(unsigned int i = 0; i < MAX_VORTEX_COUNT; i++){
			Vortex& vortex = ((Vortex*)m_computeUniformBuffers.snowflake.vortex[m_currentFrame].raw)[i];

			vortex.radius = s_baseRadius[i] * std::abs(std::sin(m_lastTime * 0.1f + s_basePhase[i]));
			vortex.force = s_baseForce[i] * std::sin(m_lastTime * 0.2f);
		}
	}

	void updateComputePushConstant() {
		m_computePushConstant.snowflakeCount = SNOWFLAKE_COUNT;
		m_computePushConstant.deltaTime = m_currentDeltaTime;
	}

	void initSceneContext() {
		ZoneScopedN("Update Graphic Transform Uniform Buffer");

		// view
		m_sceneContext.camView = g_camera.getViewMatrix();
		m_sceneContext.camProjection = glm::perspective(g_camera.getZoom(), swapChainExtent.width / (float) swapChainExtent.height, s_nearPlane, s_farPlane);
		m_sceneContext.lightView = glm::lookAt(s_lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_sceneContext.lightProjection = glm::ortho(s_shadowLeftPlane, s_shadowRightPlane, s_shadowBotPlane, s_shadowTopPlane, s_nearPlane, s_shadowFarPlane); 

		// snowflake
		{
			glm::mat4& model = m_sceneContext.snowflakeModel;
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(s_snowTranslate[0], s_snowTranslate[1], s_snowTranslate[2]));
			if(s_snowRotate[0] != 0.f || s_snowRotate[1] != 0.f || s_snowRotate[2] != 0.f)
				model = glm::rotate(model, m_lastTime * glm::radians(90.0f), glm::vec3(s_snowRotate[0], s_snowRotate[1], s_snowRotate[2]));
			model = glm::scale(model, glm::vec3(s_snowScale[0], s_snowScale[1], s_snowScale[2]));
		}

		{
			// candles
			Object objIdx = Object::CANDLE;
			tinygltf::Model& model = m_model[objIdx];
			{
				unsigned int meshCount = model.meshes.size();
				glm::mat4& candlesModel = m_sceneContext.candlesModel;
				candlesModel = glm::mat4(1.0f);
				candlesModel = glm::translate(candlesModel, glm::vec3(s_candlesTranslate[0], s_candlesTranslate[1], s_candlesTranslate[2]));
				candlesModel = glm::scale(candlesModel, glm::vec3(s_candlesScale[0], s_candlesScale[1], s_candlesScale[2]));

				for (unsigned int meshIdx = 0; meshIdx < meshCount; meshIdx++){
					// mesh local transform
					glm::mat4 localMeshModel = candlesModel * m_modelMeshTransforms[objIdx][meshIdx];
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
				instanceModel[3] = glm::vec4(m_towerInstanceRaw[i].pos.x, m_towerInstanceRaw[i].pos.y, m_towerInstanceRaw[i].pos.z, 1.0f);

				m_sceneContext.shadowInstanceModels.push_back(instanceModel);
			}
		}
	}

	void updateContext() {
		ZoneScopedN("ComputeAnimationCPU");
        auto now = std::chrono::high_resolution_clock::now();
        float currentTime = std::chrono::duration<float, std::chrono::seconds::period>(now - startTime).count();

		m_currentDeltaTime = currentTime - m_lastTime;
        m_lastTime = currentTime;

		// Not wait for fences here for more cpu-gpu parallel
		// updateGraphicUniformBuffer();
		// updateComputeUniformBuffer();
		// updateComputePushConstant();

		computeAnimation(Object::CANDLE);
		if (s_isLodUpdated) {
			generateIndexLOD();
			s_isLodUpdated = false;
		}
	}

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
			// std::cout << std::endl << ">>>>>>> New Frame Start <<<<<<<<" << std::endl;
			processInput();
			updateContext();
            drawFrame();
			FrameMark;
        }

        vkDeviceWaitIdle(m_device);
    }

	void savePipelineCache() {
		size_t dataSize{};
		vkGetPipelineCacheData(m_device, m_pipelineCache, &dataSize, nullptr);
		if(dataSize){
			char* data = new char[dataSize]();
			vkGetPipelineCacheData(m_device, m_pipelineCache, &dataSize, data);
			writeFile("../../data/cache/pipeline_cache.blob", data, dataSize);
		}
	}

	void cleanUpImGui(){
		ImGui_ImplVulkan_DestroyFontsTexture();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void cleanUpTracy(){
		TracyVkDestroy(tracyContext);
	}

	void cleanupShaders() {
		vkDestroyShaderModule(m_device, m_shaders.snowflakeVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.snowflakeVS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.snowflakeFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.snowflakeFS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.snowflakeCS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.snowflakeCS.reflection);

		vkDestroyShaderModule(m_device, m_shaders.candlesVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.candlesVS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.candlesFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.candlesFS.reflection);

		vkDestroyShaderModule(m_device, m_shaders.skyboxVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.skyboxVS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.skyboxFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.skyboxFS.reflection);

		vkDestroyShaderModule(m_device, m_shaders.floorVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.floorVS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.floorFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.floorFS.reflection);

		vkDestroyShaderModule(m_device, m_shaders.quadVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.quadVS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.bloomFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.bloomFS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.combineFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.combineFS.reflection);
		vkDestroyShaderModule(m_device, m_shaders.shadowViewportFS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.shadowViewportFS.reflection);

		vkDestroyShaderModule(m_device, m_shaders.shadowBatchVS.module, nullptr);
		spvReflectDestroyShaderModule(&m_shaders.shadowBatchVS.reflection);
	}

    void cleanUpVulkan() {
        cleanupSwapChain();

		cleanupShaders();

		savePipelineCache();
        vkDestroyPipeline(m_device, m_graphicPipelines.snowflake, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.candles.interleaved, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.candles.separated, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.floor, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.skybox, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.bloom.vertical, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.bloom.horizontal, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.combine, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.shadow.directional, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.shadow.viewport, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.snowflake, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.candles, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.floor, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.skybox, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.shadow, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.bloom, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicPipelineLayouts.combine, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.base, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.shadow, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.bloom, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.combine, nullptr);

		for(unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			releaseTransientBuffersAtCmdIdx(i);
		}

		for(Buffer& buffer : m_computeUniformBuffers.snowflake.vortex) {
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
			vmaUnmapMemory(m_allocator, buffer.allocation);
			vmaFreeMemory(m_allocator, buffer.allocation);
		}

		for(auto& buffer : m_storageBuffers.snowflake) {
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
			vmaFreeMemory(m_allocator, buffer.allocation);
		}

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorPool(m_device, imguiDescriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.snowflake, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.candles.tranformUniform, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.candles.meshMaterial, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.floor, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.skybox, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.shadow, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.bloom, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicDescriptorSetLayouts.combine, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_computeDescriptorSetLayouts.snowflake, nullptr);

		vkDestroyBuffer(m_device, m_vertexBuffers.snowflake.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_vertexBuffers.snowflake.allocation);

		vkDestroyBuffer(m_device, m_vertexBuffers.quad.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_vertexBuffers.quad.allocation);

		vkDestroyBuffer(m_device, m_vertexBuffers.cube.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_vertexBuffers.cube.allocation);

		for(unsigned int i = 0; i < m_vertexBuffers.candles.size(); i++) {
			for(unsigned int j = 0; j < m_vertexBuffers.candles[i].size(); j++) {
				vkDestroyBuffer(m_device, m_vertexBuffers.candles[i][j].buffer, nullptr);
				vmaFreeMemory(m_allocator, m_vertexBuffers.candles[i][j].allocation);
				free(m_vertexBuffers.candles[i][j].raw);
			}
		}

		vkDestroyBuffer(m_device, m_indexBuffers.snowflake.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_indexBuffers.snowflake.allocation);

		free(m_indexBuffers.quad.raw);
		vkDestroyBuffer(m_device, m_indexBuffers.quad.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_indexBuffers.quad.allocation);

		for(auto& buffer : m_indexBuffers.candles.lod0) {
			free(buffer.raw);
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
			vmaFreeMemory(m_allocator, buffer.allocation);
		}
		for(auto& buffer : m_indexBuffers.candles.lod1) {
			free(buffer.raw);
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
			vmaFreeMemory(m_allocator, buffer.allocation);
		}

		// shadow
		vkDestroyBuffer(m_device, m_vertexBuffers.shadow.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_vertexBuffers.shadow.allocation);
		free(m_vertexBuffers.shadow.raw);

		vkDestroyBuffer(m_device, m_indexBuffers.shadow.buffer, nullptr);
		vmaFreeMemory(m_allocator, m_indexBuffers.shadow.allocation);
		free(m_indexBuffers.shadow.raw);

		vkDestroyBuffer(m_device, m_graphicUniformBuffers.shadow.lightTransform.buffer, nullptr);
		vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.shadow.lightTransform.allocation);
		vmaFreeMemory(m_allocator, m_graphicUniformBuffers.shadow.lightTransform.allocation);

		vkDestroyBuffer(m_device, m_graphicUniformBuffers.shadow.perInstanceTransform.buffer, nullptr);
		vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.shadow.perInstanceTransform.allocation);
		vmaFreeMemory(m_allocator, m_graphicUniformBuffers.shadow.perInstanceTransform.allocation);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(m_device, m_graphicUniformBuffers.snowflake[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.snowflake[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.snowflake[i].allocation);

			vkDestroyBuffer(m_device, m_graphicUniformBuffers.floor[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.floor[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.floor[i].allocation);

			vkDestroyBuffer(m_device, m_graphicUniformBuffers.skybox[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.skybox[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.skybox[i].allocation);

			vkDestroyBuffer(m_device, m_graphicUniformBuffers.candles.perMeshTransform[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.candles.perMeshTransform[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.candles.perMeshTransform[i].allocation);

			vkDestroyBuffer(m_device, m_graphicUniformBuffers.candles.lightingTransform[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.candles.lightingTransform[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.candles.lightingTransform[i].allocation);

			vkDestroyBuffer(m_device, m_graphicUniformBuffers.shadow.perMeshTransform[i].buffer, nullptr);
			vmaUnmapMemory(m_allocator, m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation);
			vmaFreeMemory(m_allocator, m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation);
		}

		Object objIdx = Object::CANDLE;
		for (auto& meshImage : m_modelImages[objIdx]) {
			vkDestroyImage(m_device, meshImage.baseImage.image, nullptr);
			vkDestroyImage(m_device, meshImage.normalImage.image, nullptr);
			vkDestroyImage(m_device, meshImage.emissiveImage.image, nullptr);

			vmaFreeMemory(m_allocator, meshImage.baseImage.allocation);
			vmaFreeMemory(m_allocator, meshImage.normalImage.allocation);
			vmaFreeMemory(m_allocator, meshImage.emissiveImage.allocation);

			vkDestroyImageView(m_device, meshImage.baseImage.view, nullptr);
			vkDestroyImageView(m_device, meshImage.normalImage.view, nullptr);
			vkDestroyImageView(m_device, meshImage.emissiveImage.view, nullptr);
		}

		vkDestroyImage(m_device, m_skyboxImage.image, nullptr);
		vkFreeMemory(m_device, m_skyboxImage.memory, nullptr);
		vkDestroyImageView(m_device, m_skyboxImage.view, nullptr);

		vkDestroySampler(m_device, m_samplers.candles, nullptr);
		vkDestroySampler(m_device, m_samplers.shadow, nullptr);
		vkDestroySampler(m_device, m_samplers.postFX, nullptr);
		vkDestroySampler(m_device, m_samplers.skybox, nullptr);

        vkDestroyBuffer(m_device, m_towerInstanceBuffer, nullptr);
        vmaFreeMemory(m_allocator, instanceBufferAlloc);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_computeStartingSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_inFlightGraphicFences[i], nullptr);

			vkDestroyFence(m_device, m_inFlightComputeFences[i], nullptr);
			vkDestroySemaphore(m_device, m_computeFinishedSemaphores[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_graphicCommandPool, nullptr);
        vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
        // vkDestroyCommandPool(m_device, timestampPool, nullptr);
		vmaDestroyAllocator(m_allocator);

        vkDestroyDevice(m_device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

	void cleanupFrameBuffers() {
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyImageView(m_device, m_renderTargets[i].base.colorRT.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].base.colorRT.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].base.colorRT.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].base.colorResRT.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].base.colorResRT.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].base.colorResRT.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].base.depthRT.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].base.depthRT.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].base.depthRT.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].base.bloomThresholdRT.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].base.bloomThresholdRT.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].base.bloomThresholdRT.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].base.bloomThresholdResRT.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].base.bloomThresholdResRT.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].base.bloomThresholdResRT.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].shadow.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].shadow.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].shadow.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].bloom1.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].bloom1.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].bloom1.allocation);

			vkDestroyImageView(m_device, m_renderTargets[i].bloom2.view, nullptr);
			vkDestroyImage(m_device, m_renderTargets[i].bloom2.image, nullptr);
			vmaFreeMemory(m_allocator, m_renderTargets[i].bloom2.allocation);
		}
        for (auto framebuffer : m_frameBuffers.base) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        for (auto framebuffer : m_frameBuffers.shadow) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        for (auto framebuffer : m_frameBuffers.bloom.horizontal) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        for (auto framebuffer : m_frameBuffers.bloom.vertical) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        for (auto framebuffer : m_frameBuffers.combine) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
	}

    void cleanupSwapChain() {
		cleanupFrameBuffers();
        for (auto imageView : m_swapChainImageViews) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device);

        cleanupSwapChain();

        createSwapchainImageViews();
        createRenderTargets();
        createFramebuffers();
    }


	void createAllocator(){
		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.m_device = m_device;
		allocatorInfo.preferredLargeHeapBlockSize = 0;
		allocatorInfo.pAllocationCallbacks = nullptr;
		allocatorInfo.pDeviceMemoryCallbacks = nullptr;
		allocatorInfo.pHeapSizeLimit = nullptr;
		allocatorInfo.pVulkanFunctions = nullptr;
		allocatorInfo.instance = instance;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
            throw std::runtime_error("fail to create memory allocator");
        }
	}

    void createSwapchainImageViews() {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (uint32_t i = 0; i < m_swapChainImages.size(); i++) {
            m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void createRenderPasses() {
		// base render pass
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = m_renderTargetImageFormat;
			colorAttachment.samples = m_msaaSamples;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription bloomThresholdAttachment{};
			bloomThresholdAttachment.format = m_renderTargetImageFormat;
			bloomThresholdAttachment.samples = m_msaaSamples;
			bloomThresholdAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			bloomThresholdAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			bloomThresholdAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			bloomThresholdAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			bloomThresholdAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			bloomThresholdAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = findDepthFormat();
			depthAttachment.samples = m_msaaSamples;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription colorAttachmentResolve{};
			colorAttachmentResolve.format = m_renderTargetImageFormat;
			colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentDescription bloomThresholdAttachmentResolve{};
			bloomThresholdAttachmentResolve.format = m_renderTargetImageFormat;
			bloomThresholdAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			bloomThresholdAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			bloomThresholdAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			bloomThresholdAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			bloomThresholdAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			bloomThresholdAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			bloomThresholdAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference bloomThresholdAttachmentRef{};
			bloomThresholdAttachmentRef.attachment = 1;
			bloomThresholdAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			std::array<VkAttachmentReference, 2> colorRefs = {colorAttachmentRef, bloomThresholdAttachmentRef};

			VkAttachmentReference depthAttachmentRef{};
			depthAttachmentRef.attachment = 2;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorAttachmentResolveRef{};
			colorAttachmentResolveRef.attachment = 3;
			colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference bloomThresholdAttachmentResolveRef{};
			bloomThresholdAttachmentResolveRef.attachment = 4;
			bloomThresholdAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			std::array<VkAttachmentReference, 2> colorResolveRefs = {colorAttachmentResolveRef, bloomThresholdAttachmentResolveRef};

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = colorRefs.size();
			subpass.pColorAttachments = colorRefs.data();
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
			subpass.pResolveAttachments = colorResolveRefs.data();

			std::array<VkSubpassDependency, 2> dependencies;
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			std::array<VkAttachmentDescription, 5> attachments = {colorAttachment, bloomThresholdAttachment, 
				depthAttachment, colorAttachmentResolve, bloomThresholdAttachmentResolve};
			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = dependencies.size();
			renderPassInfo.pDependencies = dependencies.data();

			if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPasses.base) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		// shadow pass
		{
			VkAttachmentDescription shadowAttachment{};
			shadowAttachment.format = findDepthFormat();
			shadowAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			shadowAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			shadowAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			shadowAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			shadowAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			shadowAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			shadowAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference shadowRef{};
			shadowRef.attachment = 0;
			shadowRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription shadowSubpass{};
			shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			shadowSubpass.colorAttachmentCount = 0;
			shadowSubpass.pDepthStencilAttachment = &shadowRef;

			std::array<VkSubpassDependency, 2> shadowDeps{};
			shadowDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			shadowDeps[0].dstSubpass = 0;
			shadowDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			shadowDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			shadowDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			shadowDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			shadowDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			shadowDeps[1].srcSubpass = 0;
			shadowDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			shadowDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			shadowDeps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			shadowDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			shadowDeps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			shadowDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo bloomPassInfo{};
			bloomPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
			bloomPassInfo.subpassCount = 1;
			bloomPassInfo.pSubpasses = &shadowSubpass;
			bloomPassInfo.attachmentCount = 1;
			bloomPassInfo.pAttachments = &shadowAttachment;
			bloomPassInfo.dependencyCount = shadowDeps.size();
			bloomPassInfo.pDependencies = shadowDeps.data();

			if (vkCreateRenderPass(m_device, &bloomPassInfo, nullptr, &m_renderPasses.shadow) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		// bloom pass
		{
			VkAttachmentDescription blurAttachment{};
			blurAttachment.format = m_renderTargetImageFormat;
			blurAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			blurAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			blurAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			blurAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			blurAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			blurAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			blurAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference blurRef{};
			blurRef.attachment = 0;
			blurRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription blurSubpass{};
			blurSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			blurSubpass.colorAttachmentCount = 1;
			blurSubpass.pColorAttachments = &blurRef;

			std::array<VkSubpassDependency, 2> blurDeps{};
			blurDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			blurDeps[0].dstSubpass = 0;
			blurDeps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			blurDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			blurDeps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			blurDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			blurDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			blurDeps[1].srcSubpass = 0;
			blurDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			blurDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			blurDeps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			blurDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			blurDeps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			blurDeps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo bloomPassInfo{};
			bloomPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
			bloomPassInfo.subpassCount = 1;
			bloomPassInfo.pSubpasses = &blurSubpass;
			bloomPassInfo.attachmentCount = 1;
			bloomPassInfo.pAttachments = &blurAttachment;
			bloomPassInfo.dependencyCount = blurDeps.size();
			bloomPassInfo.pDependencies = blurDeps.data();

			if (vkCreateRenderPass(m_device, &bloomPassInfo, nullptr, &m_renderPasses.bloom) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		// combine pass
		{
			VkAttachmentDescription combineAttachment{};
			combineAttachment.format = m_swapchainImageFormat;
			combineAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			combineAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			combineAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			combineAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			combineAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			combineAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			combineAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference combineRef{};
			combineRef.attachment = 0;
			combineRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription combineSubpass{};
			combineSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			combineSubpass.colorAttachmentCount = 1;
			combineSubpass.pColorAttachments = &combineRef;

			std::array<VkSubpassDependency, 2> combineDeps{};
			combineDeps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			combineDeps[0].dstSubpass = 0;
			combineDeps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			combineDeps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			combineDeps[0].srcAccessMask = VK_ACCESS_NONE;
			combineDeps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			combineDeps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			combineDeps[1].srcSubpass = 0;
			combineDeps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			combineDeps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			combineDeps[1].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			combineDeps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			combineDeps[1].dstAccessMask = VK_ACCESS_NONE;
			combineDeps[1].dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;

			VkRenderPassCreateInfo combinePassInfo{};
			combinePassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
			combinePassInfo.subpassCount = 1;
			combinePassInfo.pSubpasses = &combineSubpass;
			combinePassInfo.attachmentCount = 1;
			combinePassInfo.pAttachments = &combineAttachment;
			combinePassInfo.dependencyCount = combineDeps.size();
			combinePassInfo.pDependencies = combineDeps.data();
			if (vkCreateRenderPass(m_device, &combinePassInfo, nullptr, &m_renderPasses.combine) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}
    }

    void createDescriptorSetLayouts() {
		createGraphicDescriptorSetLayouts();	
		createComputeDescriptorSetLayouts();
    }

	void createGraphicDescriptorSetLayouts() {
		// snowflake
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding{};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &uboLayoutBinding;

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.snowflake)
							, "fail to create snowflake descriptor set layout");
		}

		// candles
		{
			// for candles: 2 descriptor set layouts, 1 for texture+sampler(change for each mesh), 1 for uniform buffer (change each frame)
			{
				VkDescriptorSetLayoutBinding uboLayoutBinding{};
				uboLayoutBinding.binding = 0;
				uboLayoutBinding.descriptorCount = 1;
				uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				uboLayoutBinding.pImmutableSamplers = nullptr;
				uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

				VkDescriptorSetLayoutBinding lightBinding{};
				lightBinding.binding = 1;
				lightBinding.descriptorCount = 1;
				lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				lightBinding.pImmutableSamplers = nullptr;
				lightBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, lightBinding};
				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
				layoutInfo.pBindings = bindings.data();

				if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.candles.tranformUniform) != VK_SUCCESS) {
					throw std::runtime_error("failed to create descriptor set layout!");
				}
			}

			{
				VkDescriptorSetLayoutBinding samplerLayoutBinding{};
				samplerLayoutBinding.binding = 2;
				samplerLayoutBinding.descriptorCount = 1;
				samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				samplerLayoutBinding.pImmutableSamplers = nullptr;
				samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				VkDescriptorSetLayoutBinding normalBinding{};
				normalBinding.binding = 3;
				normalBinding.descriptorCount = 1;
				normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				normalBinding.pImmutableSamplers = nullptr;
				normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				VkDescriptorSetLayoutBinding emissiveBinding{};
				emissiveBinding.binding = 4;
				emissiveBinding.descriptorCount = 1;
				emissiveBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				emissiveBinding.pImmutableSamplers = nullptr;
				emissiveBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				std::array<VkDescriptorSetLayoutBinding, 3> bindings = {samplerLayoutBinding, normalBinding, emissiveBinding};
				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
				layoutInfo.pBindings = bindings.data();

				if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.candles.meshMaterial) != VK_SUCCESS) {
					throw std::runtime_error("failed to create descriptor set layout!");
				}
			}
		}

		// floor
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding{};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = 1;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.floor)
							, "fail to create snowflake descriptor set layout");
		}

		// skybox
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding{};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.pImmutableSamplers = nullptr;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = 1;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.skybox)
							, "fail to create snowflake descriptor set layout");
		}

		// shadow
		{
			VkDescriptorSetLayoutBinding transform;
			transform.binding = 0;
			transform.descriptorCount = 1;
			transform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			transform.pImmutableSamplers = nullptr;
			transform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding perMeshTransform;
			perMeshTransform.binding = 1;
			perMeshTransform.descriptorCount = 1;
			perMeshTransform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			perMeshTransform.pImmutableSamplers = nullptr;
			perMeshTransform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding perInstanceTransform;
			perInstanceTransform.binding = 2;
			perInstanceTransform.descriptorCount = 1;
			perInstanceTransform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			perInstanceTransform.pImmutableSamplers = nullptr;
			perInstanceTransform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding pBinding[3] = {transform, perMeshTransform, perInstanceTransform};
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 3;
			layoutInfo.pBindings = pBinding;

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.shadow)
				   , "fail to create shadow descriptor set layout");
		}

		// for bloom
		{
			VkDescriptorSetLayoutBinding baseBinding{};
			baseBinding.binding = 0;
			baseBinding.descriptorCount = 1;
			baseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			baseBinding.pImmutableSamplers = nullptr;
			baseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &baseBinding;

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.bloom)
				, "fail to create bloom descriptor set layout");
		}

		// for combine
		{
			VkDescriptorSetLayoutBinding baseBinding{};
			baseBinding.binding = 0;
			baseBinding.descriptorCount = 1;
			baseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			baseBinding.pImmutableSamplers = nullptr;
			baseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutBinding bloomBinding{};
			bloomBinding.binding = 1;
			bloomBinding.descriptorCount = 1;
			bloomBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bloomBinding.pImmutableSamplers = nullptr;
			bloomBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 2> bindings{baseBinding, bloomBinding};

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_graphicDescriptorSetLayouts.combine)
				, "fail to create bloom descriptor set layout");
		}
	}

	void createComputeDescriptorSetLayouts() {
		// snowflake
		{
			VkDescriptorSetLayoutBinding inputStorageBinding{};
			inputStorageBinding.binding = 0;
			inputStorageBinding.descriptorCount = 1;
			inputStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			inputStorageBinding.pImmutableSamplers = nullptr;
			inputStorageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayoutBinding outputStorageBinding{};
			outputStorageBinding.binding = 1;
			outputStorageBinding.descriptorCount = 1;
			outputStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			outputStorageBinding.pImmutableSamplers = nullptr;
			outputStorageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = 2;
			uboBinding.descriptorCount = 1;
			uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboBinding.pImmutableSamplers = nullptr;
			uboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			std::array<VkDescriptorSetLayoutBinding, 3> bindings = {inputStorageBinding, outputStorageBinding, uboBinding};
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeDescriptorSetLayouts.snowflake) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout!");
			}
		}
	}

    void createPipelineCache() {
		if(!isFileExist("../../data/cache/pipeline_cache.blob"))
			makeFile("../../data/cache/pipeline_cache.blob");

		pipelineCacheBlob = readFile("../../data/cache/pipeline_cache.blob");

		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCacheInfo.pInitialData = static_cast<void*>(pipelineCacheBlob.data());
		pipelineCacheInfo.initialDataSize = pipelineCacheBlob.size() * sizeof(char);


		vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);
	}

	void createPipelineLayouts() {
		createGraphicPipelineLayouts();
		createComputePipelineLayouts();
	}

	void recreatePipelines() {
        vkDeviceWaitIdle(m_device);

        vkDestroyPipeline(m_device, m_graphicPipelines.snowflake, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.candles.interleaved, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.candles.separated, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.bloom.vertical, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.bloom.horizontal, nullptr);
        vkDestroyPipeline(m_device, m_graphicPipelines.combine, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);

        createGraphicPipelines();
		createComputePipelines();
	}

	void recreateRenderTargets() {
        vkDeviceWaitIdle(m_device);

		cleanupFrameBuffers();
        vkDestroyRenderPass(m_device, m_renderPasses.base, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.bloom, nullptr);
        vkDestroyRenderPass(m_device, m_renderPasses.combine, nullptr);

		createRenderPasses();
		recreatePipelines();
		createRenderTargets();
        createFramebuffers();
		createGraphicDescriptorSets();
	}

	void createPipelines() {
        createGraphicPipelines();
		createComputePipelines();
	}

	void createGraphicPipelineLayouts() {
		// snowflake
		{
			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_graphicDescriptorSetLayouts.snowflake;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayouts.snowflake) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphic pipeline layout!");
			}
		}

		// candles
		{
			VkPushConstantRange pushConstant{};
			pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstant.size = sizeof(Float);
			pushConstant.offset = 0;

			VkDescriptorSetLayout layouts[2] = {m_graphicDescriptorSetLayouts.candles.tranformUniform, m_graphicDescriptorSetLayouts.candles.meshMaterial};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 2;
			pipelineLayoutInfo.pSetLayouts = layouts;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

			if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayouts.candles) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphic pipeline layout!");
			}
		}

		// floor
		{
			VkDescriptorSetLayout desLayouts{m_graphicDescriptorSetLayouts.floor};

			VkPipelineLayoutCreateInfo foorLayout{};
			foorLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			foorLayout.setLayoutCount = 1;
			foorLayout.pSetLayouts = &desLayouts;
			foorLayout.pushConstantRangeCount = 0;
			foorLayout.pPushConstantRanges = 0;

			if (vkCreatePipelineLayout(m_device, &foorLayout, nullptr, &m_graphicPipelineLayouts.floor) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphic pipeline layout!");
			}
		}

		// skybox
		{
			VkDescriptorSetLayout desLayouts{m_graphicDescriptorSetLayouts.skybox};

			VkPipelineLayoutCreateInfo foorLayout{};
			foorLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			foorLayout.setLayoutCount = 1;
			foorLayout.pSetLayouts = &desLayouts;
			foorLayout.pushConstantRangeCount = 0;
			foorLayout.pPushConstantRanges = 0;

			if (vkCreatePipelineLayout(m_device, &foorLayout, nullptr, &m_graphicPipelineLayouts.skybox) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphic pipeline layout!");
			}
		}

		// shadow
		{
			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_graphicDescriptorSetLayouts.shadow;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayouts.shadow) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphic pipeline layout!");
			}
		}

		// bloom
		{
			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_graphicDescriptorSetLayouts.bloom;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			CHECK_VK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayouts.bloom)
				, "fail to create bloom pipeline layout");
		}

		// combine
		{
			VkPushConstantRange pushConstant{};
			pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstant.size = sizeof(Float);
			pushConstant.offset = 0;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_graphicDescriptorSetLayouts.combine;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

			CHECK_VK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphicPipelineLayouts.combine)
				, "fail to create bloom pipeline layout");
		}
	}

    void createGraphicPipelines() {
		// snowflake pipeline
		{
			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkVertexInputBindingDescription posBinding{};
			posBinding.binding = 0;
			posBinding.stride = 12;
			posBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription posAttribute{};
			posAttribute.binding = 0;
			posAttribute.location = 0;
			posAttribute.offset = 0;
			posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;

			auto instanceBindingDescription = VertexInstance::getBindingDescription();
			auto instanceAttributeDescription = VertexInstance::getAttributeDescriptions();
			instanceBindingDescription.binding = 1;
			instanceAttributeDescription[0].binding = 1;
			instanceAttributeDescription[0].location = 1;

			std::array<VkVertexInputBindingDescription, 2> bindings{posBinding, instanceBindingDescription};
			std::array<VkVertexInputAttributeDescription, 2> attributes{posAttribute, instanceAttributeDescription[0]};

			vertexInputInfo.vertexBindingDescriptionCount = bindings.size();
			vertexInputInfo.pVertexBindingDescriptions = bindings.data();
			vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
			vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

			VkPipelineVertexInputDivisorStateCreateInfoEXT divisor{};
			divisor.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;

			VkVertexInputBindingDivisorDescriptionEXT divisorDescription{};
			divisorDescription.binding = 1;
			divisorDescription.divisor = 1;

			divisor.vertexBindingDivisorCount = 1;
			divisor.pVertexBindingDivisors = &divisorDescription;

			vertexInputInfo.pNext = &divisor;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = m_msaaSamples;

			VkPipelineDepthStencilStateCreateInfo depthStencil{};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			// 2 attachments for 2 framebuffer attachments
			std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments{colorBlendAttachment, colorBlendAttachment};

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
			colorBlending.pAttachments = blendAttachments.data();
			// colorBlending.blendConstants[0] = 0.0f;
			// colorBlending.blendConstants[1] = 0.0f;
			// colorBlending.blendConstants[2] = 0.0f;
			// colorBlending.blendConstants[3] = 0.0f;

			std::array<VkDynamicState, 2> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
			};
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.snowflakeVS.module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = m_shaders.snowflakeFS.module;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = m_graphicPipelineLayouts.snowflake;
			pipelineInfo.renderPass = m_renderPasses.base;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			if (vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.snowflake) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}
		}

			// candles pipeline
		{
			tinygltf::Model& model = m_model[Object::CANDLE];
			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			auto vertexDef = getModelVertexDescriptions(Object::CANDLE);

			auto instanceBindingDescription = VertexInstance::getBindingDescription();
			auto instanceAttributeDescription = VertexInstance::getAttributeDescriptions();

			std::array<VkVertexInputBindingDescription, 5> BindingDescriptions = 
				{vertexDef[0].first, vertexDef[1].first, vertexDef[2].first, vertexDef[3].first, instanceBindingDescription};
			std::array<VkVertexInputAttributeDescription, 5> AttributeDescriptions = 
				{vertexDef[0].second, vertexDef[1].second, vertexDef[2].second, vertexDef[3].second, instanceAttributeDescription[0]};

			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(BindingDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = BindingDescriptions.data();
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

			VkPipelineVertexInputDivisorStateCreateInfoEXT divisor{};
			divisor.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;

			VkVertexInputBindingDivisorDescriptionEXT divisorDescription{};
			divisorDescription.binding = 4;
			divisorDescription.divisor = 1;

			divisor.vertexBindingDivisorCount = 1;
			divisor.pVertexBindingDivisors = &divisorDescription;

			vertexInputInfo.pNext = &divisor;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = m_msaaSamples;

			VkPipelineDepthStencilStateCreateInfo depthStencil{};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MIN;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

			// 2 attachments for 2 framebuffer attachments
			std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments{colorBlendAttachment, colorBlendAttachment};

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
			colorBlending.pAttachments = blendAttachments.data();
			// colorBlending.blendConstants[0] = 0.0f;
			// colorBlending.blendConstants[1] = 0.0f;
			// colorBlending.blendConstants[2] = 0.0f;
			// colorBlending.blendConstants[3] = 0.0f;

			std::array<VkDynamicState, 3> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
				VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT
			};
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			struct SpecializationConstant{
				alignas(4) bool useTexture{true};
			}specConstant;

			std::array<VkSpecializationMapEntry, 1> specEntries;
			specEntries[0].constantID = 0;
			specEntries[0].offset = 0;
			specEntries[0].size = sizeof(SpecializationConstant);

			VkSpecializationInfo specInfo{};
			specInfo.mapEntryCount = static_cast<uint32_t>(specEntries.size());
			specInfo.pMapEntries = specEntries.data();
			specInfo.dataSize = sizeof(SpecializationConstant);
			specInfo.pData = &specConstant;

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.candlesVS.module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = m_shaders.candlesFS.module;
			fragShaderStageInfo.pName = "main";
			fragShaderStageInfo.pSpecializationInfo = &specInfo;

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = m_graphicPipelineLayouts.candles;
			pipelineInfo.renderPass = m_renderPasses.base;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			if(Object::CANDLE == Object::SNOWFLAKE)
				specConstant.useTexture = false;

			if (vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.candles.separated) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			// same pipeline but with interleaved attribute for non-animated optimized meshes
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			auto interleavedVertexDef = getInterleavedVertexDescriptions(Object::CANDLE);
			instanceBindingDescription.binding = 1;
			instanceAttributeDescription[0].binding = 1;
			divisorDescription.binding = 1;

			std::array<VkVertexInputBindingDescription, 2> interBindingDescriptions {interleavedVertexDef.first, instanceBindingDescription};
			std::array<VkVertexInputAttributeDescription, 5> interAttributeDescriptions = 
				{interleavedVertexDef.second[0], interleavedVertexDef.second[1], interleavedVertexDef.second[2]
					, interleavedVertexDef.second[3], instanceAttributeDescription[0]};

			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(interBindingDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = interBindingDescriptions.data();
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(interAttributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = interAttributeDescriptions.data();

			pipelineInfo.pVertexInputState = &vertexInputInfo;
			if (vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.candles.interleaved) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}
		}

		// floor
		{ 
			VkVertexInputBindingDescription vertexBindings{};
			vertexBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindings.binding = 0;
			vertexBindings.stride = 5 * sizeof(float);

			VkVertexInputAttributeDescription vertexAttributePos{};
			vertexAttributePos.binding = 0;
			vertexAttributePos.location = 0;
			vertexAttributePos.offset = 0;
			vertexAttributePos.format = VK_FORMAT_R32G32B32_SFLOAT;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &vertexBindings;
			vertexInputInfo.vertexAttributeDescriptionCount = 1;
			vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributePos;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			// inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.scissorCount = 1;
			viewportInfo.viewportCount = 1;
			// ignore this since viewport state is dynamic
			// viewportInfo.pScissors
			// viewportInfo.pViewports

			VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
			rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationInfo.lineWidth = 1.0f;
			rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
			rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationInfo.depthBiasEnable = VK_FALSE;
			rasterizationInfo.depthClampEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.rasterizationSamples = m_msaaSamples;
			multisampleInfo.sampleShadingEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthTestEnable = VK_TRUE;
			depthStencilInfo.depthWriteEnable = VK_TRUE;
			depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilInfo.stencilTestEnable = VK_FALSE;
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			// 2 attachments for 2 framebuffer attachments
			std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments{colorBlendAttachment, colorBlendAttachment};

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.logicOpEnable = VK_FALSE;
			blendInfo.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
			blendInfo.pAttachments = blendAttachments.data();

			std::array<VkDynamicState, 2> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = dynamicStates.size();
			dynamicInfo.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.floorVS.module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = m_shaders.floorFS.module;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.pMultisampleState = &multisampleInfo;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
			pipelineInfo.pRasterizationState = &rasterizationInfo;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.renderPass = m_renderPasses.base;
			pipelineInfo.subpass = 0;
			pipelineInfo.layout = m_graphicPipelineLayouts.floor;

			CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.floor)
				   , "fail to create floor pipeline");

		}

		// skybox
		{ 
			VkVertexInputBindingDescription vertexBindings{};
			vertexBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindings.binding = 0;
			vertexBindings.stride = 3 * sizeof(float);

			VkVertexInputAttributeDescription vertexAttributePos{};
			vertexAttributePos.binding = 0;
			vertexAttributePos.location = 0;
			vertexAttributePos.offset = 0;
			vertexAttributePos.format = VK_FORMAT_R32G32B32_SFLOAT;

			std::array<VkVertexInputAttributeDescription, 1> attrs{vertexAttributePos};

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &vertexBindings;
			vertexInputInfo.vertexAttributeDescriptionCount = attrs.size();
			vertexInputInfo.pVertexAttributeDescriptions = attrs.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.scissorCount = 1;
			viewportInfo.viewportCount = 1;
			// ignore this since viewport state is dynamic
			// viewportInfo.pScissors
			// viewportInfo.pViewports

			VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
			rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationInfo.lineWidth = 1.0f;
			rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
			rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationInfo.depthBiasEnable = VK_FALSE;
			rasterizationInfo.depthClampEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.rasterizationSamples = m_msaaSamples;
			multisampleInfo.sampleShadingEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthTestEnable = VK_TRUE;
			depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilInfo.depthWriteEnable = VK_FALSE;
			depthStencilInfo.stencilTestEnable = VK_FALSE;
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			// 2 attachments for 2 framebuffer attachments
			std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments{colorBlendAttachment, colorBlendAttachment};

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.logicOpEnable = VK_FALSE;
			blendInfo.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
			blendInfo.pAttachments = blendAttachments.data();

			std::array<VkDynamicState, 2> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = dynamicStates.size();
			dynamicInfo.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.skyboxVS.module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = m_shaders.skyboxFS.module;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.pMultisampleState = &multisampleInfo;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
			pipelineInfo.pRasterizationState = &rasterizationInfo;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.renderPass = m_renderPasses.base;
			pipelineInfo.subpass = 0;
			pipelineInfo.layout = m_graphicPipelineLayouts.skybox;

			CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.skybox)
				   , "fail to create skybox pipeline");
		}

		// shadow
		{
			VkVertexInputBindingDescription	 vertexBinding{};
			vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBinding.binding = 0;
			vertexBinding.stride = 4 * sizeof(float);

			VkVertexInputAttributeDescription vertexAttr{};
			vertexAttr.binding = 0;
			vertexAttr.location = 0;
			vertexAttr.location = 0;
			vertexAttr.format = VK_FORMAT_R32G32B32A32_SFLOAT;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
			vertexInputInfo.vertexAttributeDescriptionCount = 1;
			vertexInputInfo.pVertexAttributeDescriptions = &vertexAttr;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.scissorCount = 1;
			viewportInfo.viewportCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
			rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationInfo.lineWidth = 1.0f;
			rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
			rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationInfo.depthBiasEnable = VK_FALSE;
			rasterizationInfo.depthClampEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleInfo.sampleShadingEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthTestEnable = VK_TRUE;
			depthStencilInfo.depthWriteEnable = VK_TRUE;
			depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
			depthStencilInfo.minDepthBounds = 0;
			depthStencilInfo.maxDepthBounds = 0;
			depthStencilInfo.stencilTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState blendAttachmentInfo{};
			// WTF: we have to set value for this `colorWriteMask` flag for some reason???
			blendAttachmentInfo.colorWriteMask = 0;
			blendAttachmentInfo.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.attachmentCount = 1;
			blendInfo.pAttachments = &blendAttachmentInfo;

			std::array<VkDynamicState, 2> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = dynamicStates.size();
			dynamicInfo.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.shadowBatchVS.module;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.pMultisampleState = &multisampleInfo;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
			pipelineInfo.pRasterizationState = &rasterizationInfo;
			pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(shaderStages[0]);
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.renderPass = m_renderPasses.shadow;
			pipelineInfo.subpass = 0;
			pipelineInfo.layout = m_graphicPipelineLayouts.shadow;

			CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.shadow.directional)
				   , "fail to create shadow pipeline");
		}

		// bloom & combine & shadow view pipeline
		{ 
			// shared states
			VkVertexInputBindingDescription vertexBindings{};
			vertexBindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindings.binding = 0;
			vertexBindings.stride = 5 * sizeof(float);

			VkVertexInputAttributeDescription vertexAttributePos{};
			vertexAttributePos.binding = 0;
			vertexAttributePos.location = 0;
			vertexAttributePos.offset = 0;
			vertexAttributePos.format = VK_FORMAT_R32G32B32_SFLOAT;

			VkVertexInputAttributeDescription vertexAttributeTex{};
			vertexAttributeTex.binding = 0;
			vertexAttributeTex.location = 1;
			vertexAttributeTex.offset = 12;
			vertexAttributeTex.format = VK_FORMAT_R32G32_SFLOAT;

			std::array<VkVertexInputAttributeDescription, 2> vertexAttribute{vertexAttributePos, vertexAttributeTex};

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &vertexBindings;
			vertexInputInfo.vertexAttributeDescriptionCount = vertexAttribute.size();
			vertexInputInfo.pVertexAttributeDescriptions = vertexAttribute.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			// inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.scissorCount = 1;
			viewportInfo.viewportCount = 1;
			// ignore this since viewport state is dynamic
			// viewportInfo.pScissors
			// viewportInfo.pViewports

			VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
			rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationInfo.lineWidth = 1.0f;
			rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
			rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationInfo.depthBiasEnable = VK_FALSE;
			rasterizationInfo.depthClampEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleInfo.sampleShadingEnable = VK_FALSE;

			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthTestEnable = VK_FALSE;
			depthStencilInfo.depthWriteEnable = VK_FALSE;
			depthStencilInfo.stencilTestEnable = VK_FALSE;
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState blendAttachmentInfo{};
			// WTF: we have to set value for this `colorWriteMask` flag for some reason???
			blendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
			blendAttachmentInfo.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.attachmentCount = 1;
			blendInfo.pAttachments = &blendAttachmentInfo;

			std::array<VkDynamicState, 2> dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = dynamicStates.size();
			dynamicInfo.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = m_shaders.quadVS.module;
			vertShaderStageInfo.pName = "main";

			struct SpecializationConstant{
				alignas(4) int isHorizontal{0};
			} specConstant;

			std::array<VkSpecializationMapEntry, 1> specEntries;
			specEntries[0].constantID = 0;
			specEntries[0].offset = 0;
			specEntries[0].size = sizeof(SpecializationConstant);

			VkSpecializationInfo specInfo{};
			specInfo.mapEntryCount = specEntries.size();
			specInfo.pMapEntries = specEntries.data();
			specInfo.dataSize = sizeof(SpecializationConstant);
			specInfo.pData = &specConstant;

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = m_shaders.bloomFS.module;
			fragShaderStageInfo.pName = "main";
			fragShaderStageInfo.pSpecializationInfo = &specInfo;

			VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.pMultisampleState = &multisampleInfo;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
			pipelineInfo.pRasterizationState = &rasterizationInfo;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.renderPass = m_renderPasses.bloom;
			pipelineInfo.subpass = 0;
			pipelineInfo.layout = m_graphicPipelineLayouts.bloom;

			// vertical bloom pass
			{
				specConstant.isHorizontal = 0;
				CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.bloom.vertical)
					   , "fail to create bloom pipeline");
			}

			// horizontal bloom pass
			{
				specConstant.isHorizontal = 1;
				CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.bloom.horizontal)
					   , "fail to create bloom pipeline");
			}

			// combine pass
			{
				VkPipelineShaderStageCreateInfo combineVertShaderStageInfo{};
				combineVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				combineVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				combineVertShaderStageInfo.module = m_shaders.quadVS.module;
				combineVertShaderStageInfo.pName = "main";

				VkPipelineShaderStageCreateInfo combineFragShaderStageInfo{};
				combineFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				combineFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				combineFragShaderStageInfo.module = m_shaders.combineFS.module;
				combineFragShaderStageInfo.pName = "main";

				VkPipelineShaderStageCreateInfo combineShaderStages[] = {combineVertShaderStageInfo, combineFragShaderStageInfo};

				pipelineInfo.stageCount = 2;
				pipelineInfo.pStages = combineShaderStages;
				pipelineInfo.renderPass = m_renderPasses.combine;
				pipelineInfo.subpass = 0;
				pipelineInfo.layout = m_graphicPipelineLayouts.combine;

				CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.combine)
					   , "fail to create combine pipeline");

			}

			// shadow view
			{
				VkPipelineShaderStageCreateInfo shadowViewVertShaderStageInfo{};
				shadowViewVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shadowViewVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				shadowViewVertShaderStageInfo.module = m_shaders.quadVS.module;
				shadowViewVertShaderStageInfo.pName = "main";

				VkPipelineShaderStageCreateInfo shadowViewFragShaderStageInfo{};
				shadowViewFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shadowViewFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				shadowViewFragShaderStageInfo.module = m_shaders.shadowViewportFS.module;
				shadowViewFragShaderStageInfo.pName = "main";

				VkPipelineShaderStageCreateInfo shadowViewShaderStages[] = {shadowViewVertShaderStageInfo, shadowViewFragShaderStageInfo};

				pipelineInfo.stageCount = 2;
				pipelineInfo.pStages = shadowViewShaderStages;
				pipelineInfo.renderPass = m_renderPasses.combine;
				pipelineInfo.subpass = 0;
				pipelineInfo.layout = m_graphicPipelineLayouts.bloom;

				CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_graphicPipelines.shadow.viewport)
					   , "fail to create shadowView pipeline");
			}
		}
    }

	void createComputePipelineLayouts() {
		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstant.size = sizeof(ComputePushConstant);
		pushConstant.offset = 0;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayouts.snowflake;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create compute pipeline layout!");
	}

	void createComputePipelines() {
		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = m_shaders.snowflakeCS.module;
        computeShaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.flags = 0;
		pipelineInfo.stage = computeShaderStageInfo;
		pipelineInfo.layout = m_computePipelineLayout;

		if (vkCreateComputePipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create compute pipeline!");
	}

    void createFramebuffers() {

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			// base
			std::array<VkImageView, 5> attachments = {
				m_renderTargets[i].base.colorRT.view,
				m_renderTargets[i].base.bloomThresholdRT.view,
				m_renderTargets[i].base.depthRT.view,
				m_renderTargets[i].base.colorResRT.view,
				m_renderTargets[i].base.bloomThresholdResRT.view
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPasses.base;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffers.base[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			// shadow
			VkFramebufferCreateInfo shadowFOInfo{};
			shadowFOInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			shadowFOInfo.renderPass = m_renderPasses.shadow;
			shadowFOInfo.attachmentCount = 1;
			shadowFOInfo.pAttachments = &m_renderTargets[i].shadow.view;
			shadowFOInfo.width = m_shadowExtent.width;
			shadowFOInfo.height = m_shadowExtent.height;
			shadowFOInfo.layers = 1;
			if (vkCreateFramebuffer(m_device, &shadowFOInfo, nullptr, &m_frameBuffers.shadow[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			// bloom
			std::array<VkImageView, 1> bloom1Attachments = {
				m_renderTargets[i].bloom1.view
			};

			VkFramebufferCreateInfo bloomFBInfo{};
			bloomFBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			bloomFBInfo.renderPass = m_renderPasses.bloom;
			bloomFBInfo.attachmentCount = static_cast<uint32_t>(bloom1Attachments.size());
			bloomFBInfo.pAttachments = bloom1Attachments.data();
			bloomFBInfo.width = swapChainExtent.width;
			bloomFBInfo.height = swapChainExtent.height;
			bloomFBInfo.layers = 1;
			if (vkCreateFramebuffer(m_device, &bloomFBInfo, nullptr, &m_frameBuffers.bloom.horizontal[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}

			std::array<VkImageView, 1> bloom2Attachments = {
				m_renderTargets[i].bloom2.view,
			};
			bloomFBInfo.pAttachments = bloom2Attachments.data();
			if (vkCreateFramebuffer(m_device, &bloomFBInfo, nullptr, &m_frameBuffers.bloom.vertical[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}

		m_frameBuffers.combine.resize(m_swapChainImageViews.size());
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
			// combine
			std::array<VkImageView, 1> combineAttachments = {
				m_swapChainImageViews[i]
			};
			VkFramebufferCreateInfo combineFBInfo{};
			combineFBInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			combineFBInfo.renderPass = m_renderPasses.combine;
			combineFBInfo.attachmentCount = static_cast<uint32_t>(combineAttachments.size());
			combineFBInfo.pAttachments = combineAttachments.data();
			combineFBInfo.width = swapChainExtent.width;
			combineFBInfo.height = swapChainExtent.height;
			combineFBInfo.layers = 1;

			if (vkCreateFramebuffer(m_device, &combineFBInfo, nullptr, &m_frameBuffers.combine[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
    }

    void createCommandPools() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo graphicPoolInfo{};
        graphicPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphicPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicFamily.value();

        if (vkCreateCommandPool(m_device, &graphicPoolInfo, nullptr, &m_graphicCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }

		VkCommandPoolCreateInfo computePoolInfo{};
		computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		computePoolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();

		if (vkCreateCommandPool(m_device, &computePoolInfo, nullptr, &m_computeCommandPool) != VK_SUCCESS){
			throw std::runtime_error("failed to create compute command pool!");
		}
    }

    void createRenderTargets() {
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			// base
			createImage(swapChainExtent.width, swapChainExtent.height, 1, m_msaaSamples, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].base.colorRT.image, m_renderTargets[i].base.colorRT.allocation);
			m_renderTargets[i].base.colorRT.view = createImageView(m_renderTargets[i].base.colorRT.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			createImage(swapChainExtent.width, swapChainExtent.height, 1, m_msaaSamples, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].base.bloomThresholdRT.image, m_renderTargets[i].base.bloomThresholdRT.allocation);
			m_renderTargets[i].base.bloomThresholdRT.view = createImageView(m_renderTargets[i].base.bloomThresholdRT.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			createImage(swapChainExtent.width, swapChainExtent.height, 1, m_msaaSamples, m_depthFormat,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].base.depthRT.image, m_renderTargets[i].base.depthRT.allocation);
			m_renderTargets[i].base.depthRT.view = createImageView(m_renderTargets[i].base.depthRT.image, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

			createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].base.colorResRT.image, m_renderTargets[i].base.colorResRT.allocation);
			m_renderTargets[i].base.colorResRT.view = createImageView(m_renderTargets[i].base.colorResRT.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].base.bloomThresholdResRT.image, m_renderTargets[i].base.bloomThresholdResRT.allocation);
			m_renderTargets[i].base.bloomThresholdResRT.view = createImageView(m_renderTargets[i].base.bloomThresholdResRT.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			// shadow
			createImage(m_shadowExtent.width, m_shadowExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_depthFormat,
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].shadow.image, m_renderTargets[i].shadow.allocation);
			m_renderTargets[i].shadow.view = createImageView(m_renderTargets[i].shadow.image, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

			// bloom
			createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].bloom1.image, m_renderTargets[i].bloom1.allocation);
			m_renderTargets[i].bloom1.view = createImageView(m_renderTargets[i].bloom1.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_renderTargetImageFormat, 
						VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderTargets[i].bloom2.image, m_renderTargets[i].bloom2.allocation);
			m_renderTargets[i].bloom2.view = createImageView(m_renderTargets[i].bloom2.image, m_renderTargetImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
    }

    void createModelImages() {
		for (unsigned int i = 0; i < Object::COUNT; i++){
			Object objIdx = static_cast<Object>(i);
			tinygltf::Model& model = m_model[objIdx];

			MeshImages meshImages{};
			if (model.images.empty()) {
				continue;
			}

			// WARNING: recreate vk handles even if resource is the same.
			int meshIdx = 0;
			for (auto& mesh : model.meshes) {
				const tinygltf::Material& material = model.materials[mesh.primitives[0].material];

				const tinygltf::Texture& baseTexture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
				 meshImages.baseImage = createModelImageFromGltf(objIdx, baseTexture, true, false);

				if (material.normalTexture.index == -1) {
					// HACK: add a dummy image
					meshImages.normalImage = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
				}
				else {
					const tinygltf::Texture& normalTexture = model.textures[material.normalTexture.index];
					meshImages.normalImage = createModelImageFromGltf(objIdx, normalTexture, true, true);
				}

				if (material.emissiveTexture.index == -1) {
					// HACK: add a dummy image
					meshImages.emissiveImage = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
				}
				else {
					const tinygltf::Texture& emissiveTexture = model.textures[material.emissiveTexture.index];
					meshImages.emissiveImage = createModelImageFromGltf(objIdx, emissiveTexture, true, false);
				}

				m_modelImages[objIdx].push_back(meshImages);
			}
		}
    }

	Image createModelImageFromGltf(Object objIdx, const tinygltf::Texture& tex, bool isMipmap, bool isTexLinearSpace) {
		tinygltf::Image image = m_model[objIdx].images[tex.source];

		int texWidth = image.width;
		int texHeight = image.height;
		int texChannels = image.component;
		
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		unsigned char* pixels = image.image.data();
		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkFormat imageFormat{};
		if (isTexLinearSpace) {
			imageFormat = VK_FORMAT_R8G8_UNORM;
		}
		else {
			imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
		}

		VkBuffer stagingBuffer{};
		VmaAllocation stagingBufferAlloc{};
		VkImage textureImage{};
		VmaAllocation textureImageAlloc{};
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);

		void* data;
		vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(m_allocator, stagingBufferAlloc);

		createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, imageFormat, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			textureImage, textureImageAlloc);

		transitionImageLayout(textureImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
		if (isMipmap)
			generateMipmaps(textureImage, imageFormat, texWidth, texHeight, mipLevels);
		else
			transitionImageLayout(textureImage, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		//vkFreeMemory(m_device, stagingBufferMemory, nullptr);
		vmaFreeMemory(m_allocator, stagingBufferAlloc);

		VkImageView textureImageView = createImageView(textureImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		return Image{textureImage, textureImageAlloc, textureImageView};
	}

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void createSamplers() {
		// TODO: these are a fake ass sampler bro
		// if(m_model[objIdx].samplers.empty()){
		// 	m_samplers[objIdx] = VK_NULL_HANDLE;
		// 	continue;
		// }
		
		// assume there is 1 texture sampler per model
		// TODO: set sampler according to gltf model
		// tinygltf::Sampler& modelSampler = m_model[objIdx].samplers[0];

		// candles sampler
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = m_physicalDeviceProperties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
			samplerInfo.mipLodBias = 0.0f;

			if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplers.candles) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		// shadow sampler
		{
			VkFilter shadowmapFilter = isFormatFilterable(m_depthFormat, VK_IMAGE_TILING_OPTIMAL) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = shadowmapFilter;
			samplerInfo.minFilter = shadowmapFilter;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = m_physicalDeviceProperties.limits.maxSamplerAnisotropy;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplers.shadow) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shadow sampler!");
			}
		}

		// postFX sampler
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			// for bloom
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = m_physicalDeviceProperties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplers.postFX) != VK_SUCCESS) {
				throw std::runtime_error("failed to create postFX sampler!");
			}
		}
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& imageAlloc) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo	imageAllocInfo{};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		// imageAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		imageAllocInfo.priority = 1.0f;

		vmaCreateImage(m_allocator, &imageInfo, &imageAllocInfo, &image, &imageAlloc, nullptr);

        // if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to create image!");
        // }

        // VkMemoryRequirements memRequirements;
        // vkGetImageMemoryRequirements(m_device, image, &memRequirements);

        // VkMemoryAllocateInfo allocInfo{};
        // allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        // allocInfo.allocationSize = memRequirements.size;
        // allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to allocate image memory!");
        // }

        // vkBindImageMemory(m_device, image, imageMemory, 0);
    }

	void createSkyboxImage() {
		int x, y, n;
		unsigned char* firstImage = stbi_load(cubeBoxImageFiles[0], &x, &y, &n, STBI_rgb_alpha);
        VkDeviceSize imageSize = x * y * 4;
		std::cout << cubeBoxImageFiles[0] << " with imageSize: " << imageSize << "\n";

		unsigned char* cubeData = (unsigned char*)malloc(imageSize * 6);
		memcpy(cubeData, firstImage, imageSize);
        VkDeviceSize currentSize = imageSize;

		VkBuffer staggingBuffer;
		VkDeviceMemory staggingBufferMem;
		{
			for (unsigned int i = 1; i < 6; i++) {
				int x, y, n;
				unsigned char* image = stbi_load(cubeBoxImageFiles[i], &x, &y, &n, STBI_rgb_alpha);
				assert(x * y * 4 == imageSize);
				memcpy(cubeData + currentSize, image, imageSize);
				currentSize += imageSize;
				stbi_image_free(image);
			}

			VkBufferCreateInfo cubeImageBuffer{};
			cubeImageBuffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			cubeImageBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			cubeImageBuffer.size = imageSize * 6;
			cubeImageBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			CHECK_VK_RESULT(vkCreateBuffer(m_device, &cubeImageBuffer, 0, &staggingBuffer),
						  "Fail to create cube image buffer");

			VkMemoryRequirements memReq{};
			vkGetBufferMemoryRequirements(m_device, staggingBuffer, &memReq);
			std::cout << "mem req: " << (uint64_t)memReq.alignment;

			VkMemoryAllocateInfo staggingAllocInfo{};
			staggingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			assert(memReq.size == imageSize * 6);
			staggingAllocInfo.allocationSize = imageSize * 6;
			staggingAllocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			CHECK_VK_RESULT(vkAllocateMemory(m_device, &staggingAllocInfo, 0, &staggingBufferMem),
							"Fail to allocate memory for cube stagging buffer");

			vkBindBufferMemory(m_device, staggingBuffer, staggingBufferMem, 0);

			void* data;
			vkMapMemory(m_device, staggingBufferMem, 0, imageSize * 6, 0, &data);
				memcpy(data, cubeData, imageSize * 6);
			vkUnmapMemory(m_device, staggingBufferMem);

			free(cubeData);
		}

		VkDeviceMemory cubeImageMem;
		VkImage cubeImage;
		{
			VkImageCreateInfo cubeImageInfo{};
			cubeImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			cubeImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			cubeImageInfo.imageType = VK_IMAGE_TYPE_2D;

			cubeImageInfo.extent.width = x;
			cubeImageInfo.extent.height = y;
			cubeImageInfo.extent.depth = 1;
			cubeImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			cubeImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			cubeImageInfo.mipLevels = 1;
			cubeImageInfo.arrayLayers = 6;
			cubeImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			cubeImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			cubeImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			cubeImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			CHECK_VK_RESULT(vkCreateImage(m_device, &cubeImageInfo, 0, &cubeImage),
							"Fail to create cube image");

			VkMemoryRequirements memReq{};
			vkGetImageMemoryRequirements(m_device, cubeImage, &memReq);

			VkMemoryAllocateInfo imageMemInfo{};
			imageMemInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			assert(memReq.size == imageSize * 6);
			imageMemInfo.allocationSize = memReq.size;
			imageMemInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			CHECK_VK_RESULT(vkAllocateMemory(m_device, &imageMemInfo, 0, &cubeImageMem),
							"Fail to allocate memory for cube Image");
			vkBindImageMemory(m_device, cubeImage, cubeImageMem, 0);
		}

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		{
			VkImageMemoryBarrier beginBarrier{};
			beginBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			beginBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			beginBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			beginBarrier.srcAccessMask = 0;
			beginBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			beginBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			beginBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			beginBarrier.image = cubeImage;
			beginBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			beginBarrier.subresourceRange.baseMipLevel = 0;
			beginBarrier.subresourceRange.levelCount = 1;
			beginBarrier.subresourceRange.baseArrayLayer = 0;
			beginBarrier.subresourceRange.layerCount = 6;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &beginBarrier);

			VkBufferImageCopy copy{};
			copy.imageExtent.width = x;
			copy.imageExtent.height = y;
			copy.imageExtent.depth = 1;
			copy.imageSubresource.mipLevel = 0;
			copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy.imageSubresource.baseArrayLayer = 0;
			copy.imageSubresource.layerCount = 6;

			vkCmdCopyBufferToImage(commandBuffer, staggingBuffer, cubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

			VkImageMemoryBarrier endBarrier{};
			endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			beginBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			beginBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			endBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			endBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			endBarrier.image = cubeImage;
			endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			endBarrier.subresourceRange.baseMipLevel = 0;
			endBarrier.subresourceRange.levelCount = 1;
			endBarrier.subresourceRange.baseArrayLayer = 0;
			endBarrier.subresourceRange.layerCount = 6;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &endBarrier);
		}
        endSingleTimeCommands(commandBuffer);

		vkDestroyBuffer(m_device, staggingBuffer, 0);
		vkFreeMemory(m_device, staggingBufferMem, 0);
		m_skyboxImage.image = cubeImage;
		m_skyboxImage.memory = cubeImageMem;

		// Create Image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_skyboxImage.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        CHECK_VK_RESULT(vkCreateImageView(m_device, &viewInfo, nullptr, &m_skyboxImage.view), 
						"fail to create skybox view");

		// Create sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = m_physicalDeviceProperties.limits.maxSamplerAnisotropy; 

		CHECK_VK_RESULT(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_samplers.skybox),
					"fail to create skybox sampler");
	}

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		{
			// TracyVkZone(tracyContext, commandBuffer, "transferBufferToImage");

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = {0, 0, 0};
			region.imageExtent = {
				width,
				height,
				1
			};

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

        endSingleTimeCommands(commandBuffer);
    }

	std::array<std::pair<VkVertexInputBindingDescription, VkVertexInputAttributeDescription>, 4> 
		getModelVertexDescriptions(Object obj) {
		tinygltf::Model& model = m_model[obj];
		std::array<std::pair<VkVertexInputBindingDescription, VkVertexInputAttributeDescription>, 4> vertexDescription;
		unsigned int idx = 0;
		for (auto& attribute : model.meshes[0].primitives[0].attributes) {
			// each buffer binding for each attribute
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = idx;
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription attributeDescription{};

			attributeDescription.binding = idx;
			attributeDescription.location = idx;
			attributeDescription.offset = 0;

			if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC2) {
				attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
				bindingDescription.stride = 8;
			}
			else if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC3) {
				attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
				bindingDescription.stride = 12;
			}
			else if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC4) {
				attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				bindingDescription.stride = 16;
			}

			if(attribute.first == "POSITION") {
				vertexDescription[0] = {bindingDescription, attributeDescription};
			}
			else if(attribute.first == "NORMAL") {
				vertexDescription[1] = {bindingDescription, attributeDescription};
			}
			else if(attribute.first == "TANGENT") {
				vertexDescription[2] = {bindingDescription, attributeDescription};
			}
			else if(attribute.first == "TEXCOORD_0") {
				vertexDescription[3] = {bindingDescription, attributeDescription};
			}

			++idx;
		}
		return vertexDescription;
	}

	std::pair<VkVertexInputBindingDescription, std::array<VkVertexInputAttributeDescription, 4>> 
		getInterleavedVertexDescriptions(Object obj) {
		tinygltf::Model& model = m_model[obj];
		std::pair<VkVertexInputBindingDescription, std::array<VkVertexInputAttributeDescription, 4>> vertexDescription;

		vertexDescription.first.binding = 0;
		vertexDescription.first.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		unsigned int stride = 0;
		unsigned int idx = 0;
		for (auto& attribute : model.meshes[0].primitives[0].attributes) {
			VkVertexInputAttributeDescription attributeDescription{};

			attributeDescription.binding = 0;
			attributeDescription.location = idx;
			attributeDescription.offset = stride;

			if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC2) {
				attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
				stride += 8;
			}
			else if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC3) {
				attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
				stride += 12;
			}
			else if (model.accessors[attribute.second].type == TINYGLTF_TYPE_VEC4) {
				attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				stride += 16;
			}

			if(attribute.first == "POSITION") {
				vertexDescription.second[0] = attributeDescription;
			}
			else if(attribute.first == "NORMAL") {
				vertexDescription.second[1] = attributeDescription;
			}
			else if(attribute.first == "TANGENT") {
				vertexDescription.second[2] = attributeDescription;
			}
			else if(attribute.first == "TEXCOORD_0") {
				vertexDescription.second[3] = attributeDescription;
			}

			++idx;
		}

		vertexDescription.first.stride = stride;
		return vertexDescription;
	}

	std::vector<int> findModelVertexBufferView(Object obj) {
		tinygltf::Model& model = m_model[obj];
		std::vector<int> vertexViewIdx;
		for (auto& mesh : model.meshes) {
			for (auto& primitive : mesh.primitives) {
				for (auto& attribute : primitive.attributes) {
					int bufferViewIdx = model.accessors[attribute.second].bufferView;
					if (std::find(vertexViewIdx.begin(), vertexViewIdx.end(), bufferViewIdx) == vertexViewIdx.end()){
						vertexViewIdx.push_back(bufferViewIdx);
					}
				}
			}
		}
		return vertexViewIdx;
	}

	std::vector<int> findModelIndexBufferView(Object obj) {
		tinygltf::Model& model = m_model[obj];
		std::vector<int> indexViewIdx;
		for (auto& mesh : model.meshes) {
			for (auto& primitive : mesh.primitives) {
				int bufferViewIdx = model.accessors[primitive.indices].bufferView;
				if(std::find(indexViewIdx.begin(), indexViewIdx.end(), bufferViewIdx) == indexViewIdx.end()) {
					indexViewIdx.push_back(bufferViewIdx);
				}
			}
		}
		return indexViewIdx;
	}

	void optimizeMeshes() {
		tinygltf::Model model = m_model[Object::CANDLE];
		assert(m_vertexBuffers.candles.size() == model.meshes.size());
		m_indexBuffers.candles.lod0.resize(m_vertexBuffers.candles.size());
		for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
			// only generate LOD for mesh don't have animation (didn't interleave data)
			if (m_vertexBuffers.candles[meshIdx].size() != 1)
				continue;

			tinygltf::Mesh& mesh = model.meshes[meshIdx];
			tinygltf::Primitive& primitive = mesh.primitives[0];
			tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
			tinygltf::BufferView& view = model.bufferViews[indexAccessor.bufferView];
			tinygltf::Buffer& buffer = model.buffers[view.buffer];
			tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes["POSITION"]];

			unsigned int* indices = (unsigned int*)m_indexBuffers.candles.lod0[meshIdx].raw;
			const float* vertex = (float*)m_vertexBuffers.candles[meshIdx][0].raw;

			unsigned int* tempIndices = (unsigned int*) malloc(sizeof(unsigned int) * indexAccessor.count);
			meshopt_optimizeVertexCache(tempIndices, indices, indexAccessor.count, posAccessor.count);
			meshopt_optimizeOverdraw(indices, tempIndices, indexAccessor.count, vertex, posAccessor.count, 48, c_overdrawThreshold);

			unsigned int* tempVertices = (unsigned int*) malloc(m_vertexBuffers.candles[meshIdx][0].size);
			unsigned int newVertexSize = meshopt_optimizeVertexFetch(tempVertices, indices, indexAccessor.count, vertex, posAccessor.count, 48);

			free(m_vertexBuffers.candles[meshIdx][0].raw);
			unsigned int newSize = newVertexSize * 12 * sizeof(float);
			m_vertexBuffers.candles[meshIdx][0].raw = realloc(tempVertices, newSize);
			m_vertexBuffers.candles[meshIdx][0].size = newSize;
			m_vertexBuffers.candles[meshIdx][0].needTransfer = true;
			m_indexBuffers.candles.lod0[meshIdx].needTransfer = true;

			free(tempIndices);
		}
	}

	void generateIndexLOD() {
		m_indexBuffers.candles.lod1.resize(m_vertexBuffers.candles.size());
		tinygltf::Model model = m_model[Object::CANDLE];
		for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
			// only generate LOD for mesh don't have animation (only 1 interleaved buffer data)
			if (m_vertexBuffers.candles[meshIdx].size() != 1)
				continue;

			const unsigned int* indices = (unsigned int*)m_indexBuffers.candles.lod0[meshIdx].raw;
			unsigned int indexSize = m_indexBuffers.candles.lod0[meshIdx].size;
			const float* vertex = (float*)m_vertexBuffers.candles[meshIdx][0].raw;
			unsigned int vertexCount = m_vertexBuffers.candles[meshIdx][0].size / (12 * sizeof(float));

			unsigned int* des = (unsigned int*) malloc(indexSize);
			float* resultErr{};
			
			size_t newIdxSize = meshopt_simplifyWithAttributes(des, indices, indexSize / sizeof(unsigned int)
					  , vertex, vertexCount, 48 , vertex + 3, 48 , s_attrWeights, 9, nullptr, 0, s_targetError, 0, resultErr);

			assert(newIdxSize <= indexSize);
			if (m_indexBuffers.candles.lod1[meshIdx].raw != nullptr) {
				free(m_indexBuffers.candles.lod1[meshIdx].raw);
			}
			m_indexBuffers.candles.lod1[meshIdx].raw = (unsigned int*)realloc(des, newIdxSize * sizeof(unsigned int));
			m_indexBuffers.candles.lod1[meshIdx].size = newIdxSize * sizeof(unsigned int);
			m_indexBuffers.candles.lod1[meshIdx].needTransfer = true;
		}
	}

	void initVertexData() {
		{
			// Snowflake
			tinygltf::Model& model = m_model[Object::SNOWFLAKE];
			tinygltf::Mesh& mesh = model.meshes[0];
			tinygltf::Primitive& primitive = mesh.primitives[0];
			// only use position buffer view
			tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes["POSITION"]];
			tinygltf::BufferView view = model.bufferViews[posAccessor.bufferView];

			m_vertexBuffers.snowflake.raw = &model.buffers[view.buffer].data.at(0) + view.byteOffset + posAccessor.byteOffset;
			m_vertexBuffers.snowflake.size = view.byteLength;
			m_vertexBuffers.snowflake.needTransfer = true;
		}

		{
			tinygltf::Model& model = m_model[Object::CANDLE];
			m_vertexBuffers.candles.resize(model.meshes.size());
			bool meshHasAnim{false};
			for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
				auto& mesh = model.meshes[meshIdx];
				auto weights = computeWeights(Object::CANDLE, meshIdx);
				// note: if mesh has animation, use each buffers for each attributes
				// otherwise use interleaved attributes to input data to mesh optimizer
				if (!weights.empty()) {
					meshHasAnim = true;
					m_vertexBuffers.candles[meshIdx].resize(4);
				}
				else {
					meshHasAnim = false;
					m_vertexBuffers.candles[meshIdx].resize(1);
				}
				for (auto& primitive : mesh.primitives) {
					assert(mesh.primitives.size() == 1);	
					unsigned int i = 0;
					for (auto& attribute : primitive.attributes) {
						// HACK: there's no tangent for animated meshes
						auto& accessor = model.accessors[attribute.second];
						auto& bufferView = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[bufferView.buffer];

						if (meshHasAnim) {
							// each buffer per attribute data
							assert(m_vertexBuffers.candles[meshIdx].size() == 4);
							unsigned int size = accessor.count * accessor.type * 4 /* assume TINYGLTF_COMPONENT_TYPE_FLOAT*/;
							m_vertexBuffers.candles[meshIdx][i].size = size;
							m_vertexBuffers.candles[meshIdx][i].needTransfer = true;
							void* src = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
							m_vertexBuffers.candles[meshIdx][i].raw = (void*)malloc(size);
							memcpy(m_vertexBuffers.candles[meshIdx][i].raw, src, size);
							i++;
						}
						else {
							// one buffer for all attribute interleaved
							assert(m_vertexBuffers.candles[meshIdx].size() == 1);
							std::vector<float> src = interleaveAttributes(Object::CANDLE, meshIdx);
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
	}


	void createVertexBuffers() {
		// Snowflake
		{
			Buffer& snowBuffer = m_vertexBuffers.snowflake;

			if(snowBuffer.needTransfer) {
				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAlloc{};
				createBuffer(snowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
					memcpy(data, snowBuffer.raw, snowBuffer.size);
				vmaUnmapMemory(m_allocator, stagingBufferAlloc);
				createBuffer(snowBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, snowBuffer.buffer, snowBuffer.allocation);
				copyBuffer(stagingBuffer, snowBuffer.buffer, snowBuffer.size);
				snowBuffer.needTransfer = false;

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
				vmaFreeMemory(m_allocator, stagingBufferAlloc);
			}
		}
		
		// Candles
		{
			Object objIdx = Object::CANDLE;
			tinygltf::Model& model = m_model[objIdx];

			for (unsigned int meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
				assert(model.meshes.size() == m_vertexBuffers.candles.size());
				for (unsigned int attrIdx = 0; attrIdx < m_vertexBuffers.candles[meshIdx].size(); attrIdx++) {
					if (m_vertexBuffers.candles[meshIdx][attrIdx].needTransfer == false || m_vertexBuffers.candles[meshIdx][attrIdx].size == 0)
						continue;

					// Transfer vertex position animation data
					VkBuffer stagingBuffer;
					VmaAllocation stagingAlloc;
					unsigned int size = m_vertexBuffers.candles[meshIdx][attrIdx].size;

					createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAlloc);

					void* data;
					vmaMapMemory(m_allocator, stagingAlloc, &data);
						memcpy(data, m_vertexBuffers.candles[meshIdx][attrIdx].raw, static_cast<size_t>(size));
					vmaUnmapMemory(m_allocator, stagingAlloc);

					createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
					 , m_vertexBuffers.candles[meshIdx][attrIdx].buffer, m_vertexBuffers.candles[meshIdx][attrIdx].allocation);

					VkBufferCopy copyRegion{};
					copyRegion.size = size;

					copyBuffer(stagingBuffer, m_vertexBuffers.candles[meshIdx][attrIdx].buffer, size);

					vkDestroyBuffer(m_device, stagingBuffer, nullptr);
					vmaFreeMemory(m_allocator, stagingAlloc);

					m_vertexBuffers.candles[meshIdx][attrIdx].needTransfer = false;
				}
			}
		}

		// Shadow
		{
			Buffer& shadowBuffer = m_vertexBuffers.shadow;
			if(shadowBuffer.needTransfer) {
				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAlloc{};
				createBuffer(shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
					memcpy(data, shadowBuffer.raw, shadowBuffer.size);
				vmaUnmapMemory(m_allocator, stagingBufferAlloc);
				createBuffer(shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowBuffer.buffer, shadowBuffer.allocation);
				copyBuffer(stagingBuffer, shadowBuffer.buffer, shadowBuffer.size);
				shadowBuffer.needTransfer = false;

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
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
			createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);
			void* data;
			vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
				memcpy(data, quadListVertices, size);
			vmaUnmapMemory(m_allocator, stagingBufferAlloc);
			createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferAlloc);
			copyBuffer(stagingBuffer, vertexBuffer, size);

			m_vertexBuffers.quad.buffer = vertexBuffer;
			m_vertexBuffers.quad.allocation = vertexBufferAlloc;

			vkDestroyBuffer(m_device, stagingBuffer, nullptr);
			vmaFreeMemory(m_allocator, stagingBufferAlloc);
		}

		// Cube
		{
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAlloc{};

			VkBuffer vertexBuffer;
			VmaAllocation vertexBufferAlloc{};

			int size = sizeof(skyboxVertices);
			createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);
			void* data;
			vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
				memcpy(data, skyboxVertices, size);
			vmaUnmapMemory(m_allocator, stagingBufferAlloc);
			createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferAlloc);
			copyBuffer(stagingBuffer, vertexBuffer, size);

			m_vertexBuffers.cube.buffer = vertexBuffer;
			m_vertexBuffers.cube.allocation = vertexBufferAlloc;

			vkDestroyBuffer(m_device, stagingBuffer, nullptr);
			vmaFreeMemory(m_allocator, stagingBufferAlloc);
		}
	}

	void createIndexBuffers() {
		{
			// Snowflake
			Buffer& snowIdxBuffer = m_indexBuffers.snowflake;

			if (snowIdxBuffer.needTransfer) {
				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAloc{};

				createBuffer(snowIdxBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAloc, &data);
					memcpy(data, snowIdxBuffer.raw, snowIdxBuffer.size);
				vmaUnmapMemory(m_allocator, stagingBufferAloc);
				createBuffer(snowIdxBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, snowIdxBuffer.buffer, snowIdxBuffer.allocation);
				copyBuffer(stagingBuffer, snowIdxBuffer.buffer, snowIdxBuffer.size);
				snowIdxBuffer.needTransfer = false;	

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
				vmaFreeMemory(m_allocator, stagingBufferAloc);
			}
		}

		// candles lod0
		{
			for (auto& buffer : m_indexBuffers.candles.lod0) {
				if (buffer.needTransfer == false || buffer.size == 0)
					continue;

				Buffer newBuffer{};
				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAloc{};

				createBuffer(buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAloc, &data);
					memcpy(data, buffer.raw, buffer.size);
				vmaUnmapMemory(m_allocator, stagingBufferAloc);
				createBuffer(buffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.buffer, buffer.allocation);
				copyBuffer(stagingBuffer, buffer.buffer, buffer.size);

				buffer.needTransfer = false;

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
				vmaFreeMemory(m_allocator, stagingBufferAloc);
			}
		}

		// candles lod1, transfer meshopt generated data
		// raw lod data already setup in generateIndexLOD func
		{
			for (unsigned int i = 0; i < m_indexBuffers.candles.lod1.size(); i++) {
				auto& indexBuffer = m_indexBuffers.candles.lod1[i];
				if (indexBuffer.needTransfer == false || indexBuffer.size == 0)
					continue;

				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAloc{};
				// same size with LOD0 we need the biggest size possible for LOD1
				// for the need of re-allocating with different size
				uint32_t size{m_indexBuffers.candles.lod0[i].size};

				createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAloc, &data);
					memcpy(data, indexBuffer.raw, size);
				vmaUnmapMemory(m_allocator, stagingBufferAloc);
				createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer.buffer, indexBuffer.allocation);
				copyBuffer(stagingBuffer, indexBuffer.buffer, size);

				indexBuffer.needTransfer = false;

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
				vmaFreeMemory(m_allocator, stagingBufferAloc);
			}
		}

		{
			// Shadow
			Buffer& shadowBuffer = m_indexBuffers.shadow;
			if(shadowBuffer.needTransfer) {
				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAlloc{};
				createBuffer(shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAlloc);
				void* data;
				vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
					memcpy(data, shadowBuffer.raw, shadowBuffer.size);
				vmaUnmapMemory(m_allocator, stagingBufferAlloc);
				createBuffer(shadowBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowBuffer.buffer, shadowBuffer.allocation);
				copyBuffer(stagingBuffer, shadowBuffer.buffer, shadowBuffer.size);
				shadowBuffer.needTransfer = false;

				vkDestroyBuffer(m_device, stagingBuffer, nullptr);
				vmaFreeMemory(m_allocator, stagingBufferAlloc);
			}
		}
	}

	void createUniformBuffers(){
        createGraphicUniformBuffers();
		createComputeUniformBuffers();
	}

    void createGraphicUniformBuffers() {
		// snowflake
		{
			VkDeviceSize bufferSize = sizeof(SnowTransform);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
					, m_graphicUniformBuffers.snowflake[i].buffer, m_graphicUniformBuffers.snowflake[i].allocation);

				vmaMapMemory(m_allocator, m_graphicUniformBuffers.snowflake[i].allocation, &m_graphicUniformBuffers.snowflake[i].raw);
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
					createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
						, m_graphicUniformBuffers.candles.perMeshTransform[i].buffer, m_graphicUniformBuffers.candles.perMeshTransform[i].allocation);

					vmaMapMemory(m_allocator, m_graphicUniformBuffers.candles.perMeshTransform[i].allocation, &m_graphicUniformBuffers.candles.perMeshTransform[i].raw);
				}
			}

			// lighting uniform
			{
				VkDeviceSize bufferSize = sizeof(CandlesLightingTransform);

				for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
						, m_graphicUniformBuffers.candles.lightingTransform[i].buffer, m_graphicUniformBuffers.candles.lightingTransform[i].allocation);

					vmaMapMemory(m_allocator, m_graphicUniformBuffers.candles.lightingTransform[i].allocation, &m_graphicUniformBuffers.candles.lightingTransform[i].raw);
				}
			}
		}

		// floor
		{
			VkDeviceSize bufferSize = sizeof(FloorTransform);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
					, m_graphicUniformBuffers.floor[i].buffer, m_graphicUniformBuffers.floor[i].allocation);

				vmaMapMemory(m_allocator, m_graphicUniformBuffers.floor[i].allocation, &m_graphicUniformBuffers.floor[i].raw);
			}
		}

		// skybox
		{
			VkDeviceSize bufferSize = sizeof(SkyboxTransform);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
					, m_graphicUniformBuffers.skybox[i].buffer, m_graphicUniformBuffers.skybox[i].allocation);

				vmaMapMemory(m_allocator, m_graphicUniformBuffers.skybox[i].allocation, &m_graphicUniformBuffers.skybox[i].raw);
			}
		}

		// shadow
		{
			// heuristic mesh casting shadow size
			VkDeviceSize meshBufferCap = m_graphicUniformBuffers.shadow.perMeshTransform[0].size;
			assert(meshBufferCap > 0);
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				createBuffer(meshBufferCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
					, m_graphicUniformBuffers.shadow.perMeshTransform[i].buffer, m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation);

				vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.perMeshTransform[i].allocation, &m_graphicUniformBuffers.shadow.perMeshTransform[i].raw);
			}
			
			VkDeviceSize transCap = m_graphicUniformBuffers.shadow.lightTransform.size;
			assert(transCap > 0);
			createBuffer(transCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, m_graphicUniformBuffers.shadow.lightTransform.buffer, m_graphicUniformBuffers.shadow.lightTransform.allocation);

			vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.lightTransform.allocation, &m_graphicUniformBuffers.shadow.lightTransform.raw);

			VkDeviceSize perInstanceCap = m_graphicUniformBuffers.shadow.perInstanceTransform.size;
			assert(perInstanceCap > 0);
			createBuffer(perInstanceCap, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, m_graphicUniformBuffers.shadow.perInstanceTransform.buffer, m_graphicUniformBuffers.shadow.perInstanceTransform.allocation);

			vmaMapMemory(m_allocator, m_graphicUniformBuffers.shadow.perInstanceTransform.allocation, &m_graphicUniformBuffers.shadow.perInstanceTransform.raw);
		}
    }

    void createComputeUniformBuffers() {
		m_computeUniformBuffers.snowflake.vortex[0].raw = static_cast<void*>(new Vortex[MAX_VORTEX_COUNT]);
		m_computeUniformBuffers.snowflake.vortex[1].raw = static_cast<void*>(new Vortex[MAX_VORTEX_COUNT]);

		VkDeviceSize bufferSize = sizeof(Vortex) * MAX_VORTEX_COUNT;
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, m_computeUniformBuffers.snowflake.vortex[0].buffer, m_computeUniformBuffers.snowflake.vortex[0].allocation);
		vmaMapMemory(m_allocator, m_computeUniformBuffers.snowflake.vortex[0].allocation, &m_computeUniformBuffers.snowflake.vortex[0].raw);

		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, m_computeUniformBuffers.snowflake.vortex[1].buffer, m_computeUniformBuffers.snowflake.vortex[1].allocation);
		vmaMapMemory(m_allocator, m_computeUniformBuffers.snowflake.vortex[1].allocation, &m_computeUniformBuffers.snowflake.vortex[1].raw);

		for(unsigned int i = 0; i < MAX_VORTEX_COUNT; i++){
			Vortex& vortex0 = ((Vortex*)m_computeUniformBuffers.snowflake.vortex[0].raw)[i];
			Vortex& vortex1 = ((Vortex*)m_computeUniformBuffers.snowflake.vortex[1].raw)[i];
			vortex0.pos.x = vortex1.pos.x = generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
			vortex0.pos.y = vortex1.pos.y = generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
			vortex0.pos.z = vortex1.pos.z = generateRandomFloat(-VORTEX_COVER_RANGE, VORTEX_COVER_RANGE);
			vortex0.height = vortex1.height = generateRandomFloat(5.f, 10.f);

			s_basePhase[i] = generateRandomFloat(0.f, PHASE_RANGE);
			s_baseForce[i] = generateRandomFloat(MIN_FORCE, MAX_FORCE);
			s_baseRadius[i] = generateRandomFloat(MIN_RADIUS, MAX_RADIUS);
			vortex0.force = vortex1.force = s_baseForce[i];
			vortex0.radius = vortex1.radius = s_baseRadius[i];
		}
	}

	void createInstanceBuffer() {
		VkDeviceSize bufferSize = sizeof(m_towerInstanceRaw[0]) * m_towerInstanceRaw.size();
		VkBuffer stagingBuffer{};
		VmaAllocation stagingBufferAlloc{};

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingBufferAlloc);

		void* data;
		vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
		memcpy(data, m_towerInstanceRaw.data(), bufferSize);
		vmaUnmapMemory(m_allocator, stagingBufferAlloc);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_towerInstanceBuffer, instanceBufferAlloc);

		copyBuffer(stagingBuffer, m_towerInstanceBuffer, bufferSize);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vmaFreeMemory(m_allocator, stagingBufferAlloc);
	}

	void createStorageBuffer() {
		void* data = static_cast<void*>(new Snowflake[SNOWFLAKE_COUNT]);
		VkDeviceSize bufferSize = sizeof(Snowflake) * SNOWFLAKE_COUNT;
		VkBuffer stagingBuffer{};
		VmaAllocation stagingBufferAlloc{};

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingBufferAlloc);

		vmaMapMemory(m_allocator, stagingBufferAlloc, &data);
		// only work when set data when mapped like this
		for(unsigned int i = 0; i < SNOWFLAKE_COUNT; i++) {
			Snowflake& snow = ((Snowflake*)data)[i];
			snow.position.x = generateRandomFloat(-15.f, 15.f);
			snow.position.y = generateRandomFloat(-15.f, 15.f);
			snow.position.z = generateRandomFloat(-15.f, 15.f);
			snow.weight = generateRandomFloat(0.5f, 1.5f);
		}
		vmaUnmapMemory(m_allocator, stagingBufferAlloc);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			   , m_storageBuffers.snowflake[0].buffer, m_storageBuffers.snowflake[0].allocation);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			   , m_storageBuffers.snowflake[1].buffer, m_storageBuffers.snowflake[1].allocation);

		copyBuffer(stagingBuffer, m_storageBuffers.snowflake[0].buffer, bufferSize);
		copyBuffer(stagingBuffer, m_storageBuffers.snowflake[1].buffer, bufferSize);
		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vmaFreeMemory(m_allocator, stagingBufferAlloc);

	}

	// Pool use for both graphic and compute descriptors
    void createDescriptorPool() {
		unsigned int materialCount = 0;
		for (unsigned int i = 0; i < Object::COUNT; i++){
			Object objIdx = static_cast<Object>(i);
			tinygltf::Model& model = m_model[objIdx];

			materialCount += model.meshes.size();
		}

		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * Object::COUNT) * 10 + 1; // for mesh transform + light uniform +1 for compute uniform
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * materialCount) * 10 + /*for bloom */(1 + 2) * 5; // for base, normal and emissive texture + bloom texture
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(1);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		// Object::COUNT for the number of uniform buffer
		// materialCount for the number of mesh material
		// 1 for compute descriptor set
		// 1 for imgui descriptor set
		// 4 for other passes with each frame in flight
		poolInfo.maxSets = static_cast<uint32_t>((materialCount + Object::COUNT) * MAX_FRAMES_IN_FLIGHT * 10) + 3 + 4; // for graphics and compute

		if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
    }

    void createDescriptorSets() {
		createGraphicDescriptorSets();
		createComputeDescriptorSets();
	}

	void createGraphicDescriptorSets(){
		// snowflake
		{
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.snowflake, m_graphicDescriptorSetLayouts.snowflake};
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.snowflake.data())
							, "fail to allocate snowflake descriptor sets !!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_graphicUniformBuffers.snowflake[i].buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(SnowTransform);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.snowflake[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		// candles
		{
			Object objIdx = Object::CANDLE;
			if (m_modelImages.find(objIdx) == m_modelImages.end()) {
				return;
			}

			tinygltf::Model& model = m_model[objIdx];
			m_graphicDescriptorSets.candles.meshMaterial.resize(model.meshes.size());
			int meshIdx = 0;

			for (auto& mesh : model.meshes) {
				std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
					layouts = {m_graphicDescriptorSetLayouts.candles.meshMaterial, m_graphicDescriptorSetLayouts.candles.meshMaterial};
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = m_descriptorPool;
				allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
				allocInfo.pSetLayouts = layouts.data();

				if (vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.candles.meshMaterial[meshIdx].data()) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate graphic descriptor sets!");
				}

				// TODO: does it need 2 descriptor here since we only read image?
				for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

					VkDescriptorImageInfo imageInfo{};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = m_modelImages[objIdx][meshIdx].baseImage.view;
					// assume 1 sampler per object type
					imageInfo.sampler = m_samplers.candles;

					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = m_graphicDescriptorSets.candles.meshMaterial[meshIdx][i];
					descriptorWrites[0].dstBinding = 2;
					descriptorWrites[0].dstArrayElement = 0;
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[0].descriptorCount = 1;
					descriptorWrites[0].pImageInfo = &imageInfo;
					
					VkDescriptorImageInfo normalImageInfo{};
					normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					normalImageInfo.imageView = m_modelImages[objIdx][meshIdx].normalImage.view;
					// assume 1 sampler per object type
					normalImageInfo.sampler = m_samplers.candles;

					descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[1].dstSet = m_graphicDescriptorSets.candles.meshMaterial[meshIdx][i];
					descriptorWrites[1].dstBinding = 3;
					descriptorWrites[1].dstArrayElement = 0;
					descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[1].descriptorCount = 1;
					descriptorWrites[1].pImageInfo = &normalImageInfo;

					VkDescriptorImageInfo emissiveImageInfo{};
					emissiveImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					emissiveImageInfo.imageView = m_modelImages[objIdx][meshIdx].emissiveImage.view;
					// assume 1 sampler per object type
					emissiveImageInfo.sampler = m_samplers.candles;

					descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[2].dstSet = m_graphicDescriptorSets.candles.meshMaterial[meshIdx][i];
					descriptorWrites[2].dstBinding = 4;
					descriptorWrites[2].dstArrayElement = 0;
					descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[2].descriptorCount = 1;
					descriptorWrites[2].pImageInfo = &emissiveImageInfo;

					vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
				meshIdx++;
			}

			// allocate and update data for OBJECT UNIFORM tranform
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.candles.tranformUniform, m_graphicDescriptorSetLayouts.candles.tranformUniform};
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			if (vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.candles.tranformUniform.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate graphic descriptor sets!");
			}

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_graphicUniformBuffers.candles.perMeshTransform[i].buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(CandlesPerMeshTransform);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.candles.tranformUniform[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				VkDescriptorBufferInfo lightBufferInfo{};
				lightBufferInfo.buffer = m_graphicUniformBuffers.candles.lightingTransform[i].buffer;
				lightBufferInfo.offset = 0;
				lightBufferInfo.range = sizeof(CandlesLightingTransform);

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_graphicDescriptorSets.candles.tranformUniform[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &lightBufferInfo;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		// floor
		{
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.floor, m_graphicDescriptorSetLayouts.floor};
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.floor.data())
							, "fail to allocate snowflake descriptor sets !!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_graphicUniformBuffers.floor[i].buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(FloorTransform);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.floor[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				VkDescriptorImageInfo shadowImage{};
				shadowImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				shadowImage.imageView = m_renderTargets[i].shadow.view;
				shadowImage.sampler = m_samplers.shadow;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_graphicDescriptorSets.floor[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &shadowImage;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		// skybox
		{
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.skybox, m_graphicDescriptorSetLayouts.skybox};
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.skybox.data())
							, "fail to allocate skybox descriptor sets !!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = m_graphicUniformBuffers.skybox[i].buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(SkyboxTransform);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.skybox[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				VkDescriptorImageInfo skyboxImage{};
				skyboxImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				skyboxImage.imageView = m_skyboxImage.view;
				skyboxImage.sampler = m_samplers.skybox;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_graphicDescriptorSets.skybox[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &skyboxImage;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		// shadow
		{
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.shadow, m_graphicDescriptorSetLayouts.shadow};
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.shadow.directional.data())
							, "fail to allocate snowflake descriptor sets !!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

				VkDescriptorBufferInfo transformBufferInfo{};
				transformBufferInfo.buffer = m_graphicUniformBuffers.shadow.lightTransform.buffer;
				transformBufferInfo.offset = 0;
				transformBufferInfo.range = sizeof(ShadowLightingTransform);

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.shadow.directional[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &transformBufferInfo;

				VkDescriptorBufferInfo perMeshTransformInfo{};
				perMeshTransformInfo.buffer = m_graphicUniformBuffers.shadow.perMeshTransform[i].buffer;
				perMeshTransformInfo.offset = 0;
				perMeshTransformInfo.range = VK_WHOLE_SIZE;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_graphicDescriptorSets.shadow.directional[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &perMeshTransformInfo;

				VkDescriptorBufferInfo lookupBufferInfo{};
				lookupBufferInfo.buffer = m_graphicUniformBuffers.shadow.perInstanceTransform.buffer;
				lookupBufferInfo.offset = 0;
				lookupBufferInfo.range = VK_WHOLE_SIZE;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = m_graphicDescriptorSets.shadow.directional[i];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pBufferInfo = &lookupBufferInfo;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}

			// shadow view have the samve layout with bloom descriptor set
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				viewLayouts = {m_graphicDescriptorSetLayouts.bloom, m_graphicDescriptorSetLayouts.bloom};
			VkDescriptorSetAllocateInfo shadowViewAllocInfo{};
			shadowViewAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			shadowViewAllocInfo.descriptorPool = m_descriptorPool;
			shadowViewAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			shadowViewAllocInfo.pSetLayouts = viewLayouts.data();

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &shadowViewAllocInfo, m_graphicDescriptorSets.shadow.viewport.data())
							, "fail to allocate snowflake descriptor sets !!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				std::array<VkWriteDescriptorSet, 1> viewDescriptorWrites{};

				VkDescriptorImageInfo shadowImage{};
				shadowImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				shadowImage.imageView = m_renderTargets[i].shadow.view;
				shadowImage.sampler = m_samplers.shadow;

				viewDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				viewDescriptorWrites[0].dstSet = m_graphicDescriptorSets.shadow.viewport[i];
				viewDescriptorWrites[0].dstBinding = 0;
				viewDescriptorWrites[0].dstArrayElement = 0;
				viewDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				viewDescriptorWrites[0].descriptorCount = 1;
				viewDescriptorWrites[0].pImageInfo = &shadowImage;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(viewDescriptorWrites.size()), viewDescriptorWrites.data(), 0, nullptr);
			}
		}

		// bloom
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.bloom, m_graphicDescriptorSetLayouts.bloom};
			allocInfo.pSetLayouts = layouts.data();
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.descriptorPool = m_descriptorPool;

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.bloom1.data())
				, "failed to allocate bloom1 graphic descriptor sets!");

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.bloom2.data())
				, "failed to allocate bloom2 graphic descriptor sets!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

				VkDescriptorImageInfo bloom1ImageInfo{};
				bloom1ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				bloom1ImageInfo.imageView = m_renderTargets[i].base.bloomThresholdResRT.view;
				bloom1ImageInfo.sampler = m_samplers.postFX;

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.bloom1[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &bloom1ImageInfo;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

				// for bloom2
				VkDescriptorImageInfo bloom2ImageInfo{};
				bloom2ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				bloom2ImageInfo.imageView = m_renderTargets[i].bloom1.view;
				bloom2ImageInfo.sampler = m_samplers.postFX;

				descriptorWrites[0].pImageInfo = &bloom2ImageInfo;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.bloom2[i];
				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}

		// combine
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> 
				layouts = {m_graphicDescriptorSetLayouts.combine, m_graphicDescriptorSetLayouts.combine};
			allocInfo.pSetLayouts = layouts.data();
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.descriptorPool = m_descriptorPool;

			CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, m_graphicDescriptorSets.combine.data())
				, "failed to allocate graphic descriptor sets!");

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorImageInfo baseImageInfo{};
				baseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				baseImageInfo.imageView = m_renderTargets[i].base.colorResRT.view;
				baseImageInfo.sampler = m_samplers.postFX;

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_graphicDescriptorSets.combine[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &baseImageInfo;
                                 
				VkDescriptorImageInfo bloomImageInfo{};
				bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				bloomImageInfo.imageView = m_renderTargets[i].bloom2.view;
				bloomImageInfo.sampler = m_samplers.postFX;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_graphicDescriptorSets.combine[i]; 
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = &bloomImageInfo;

				vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}
	}

	void createComputeDescriptorSets() {
		std::array<VkDescriptorSetLayout, 2> 
			layouts = {m_computeDescriptorSetLayouts.snowflake, m_computeDescriptorSetLayouts.snowflake};

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = layouts.size();
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(m_device, &allocInfo, m_computeDescriptorSets.snowflake.data()) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate compute descriptor sets!");

		VkDescriptorBufferInfo inputStorageBufferInfo{};
		inputStorageBufferInfo.buffer = m_storageBuffers.snowflake[0].buffer;
		inputStorageBufferInfo.offset = 0;
		inputStorageBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo outputStorageBufferInfo{};
		outputStorageBufferInfo.buffer = m_storageBuffers.snowflake[1].buffer;
		outputStorageBufferInfo.offset = 0;
		outputStorageBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo uboBufferInfo{};
		uboBufferInfo.buffer = m_computeUniformBuffers.snowflake.vortex[0].buffer;
		uboBufferInfo.offset = 0;
		uboBufferInfo.range = VK_WHOLE_SIZE;

		std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

		// write for the first frame descriptorset
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_computeDescriptorSets.snowflake[0];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &inputStorageBufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_computeDescriptorSets.snowflake[0];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &outputStorageBufferInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_computeDescriptorSets.snowflake[0];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &uboBufferInfo;

		// write for the second frame descriptorset
		uboBufferInfo.buffer = m_computeUniformBuffers.snowflake.vortex[1].buffer;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = m_computeDescriptorSets.snowflake[1];
		descriptorWrites[3].dstBinding = 0;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &outputStorageBufferInfo;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = m_computeDescriptorSets.snowflake[1];
		descriptorWrites[4].dstBinding = 1;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &inputStorageBufferInfo;

		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstSet = m_computeDescriptorSets.snowflake[1];
		descriptorWrites[5].dstBinding = 2;
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pBufferInfo = &uboBufferInfo;


		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, 0);
	}

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo	allocInfo{};
		if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			allocInfo.priority = 0.0f;
		}
		else
		{
			// allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
			allocInfo.priority = 1.0f;
		}

		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.requiredFlags = 0;
		allocInfo.preferredFlags = properties;
		allocInfo.memoryTypeBits = 0;
		allocInfo.pool = VK_NULL_HANDLE;
		allocInfo.pUserData = nullptr;

		if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

        // if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to create buffer!");
        // }

        // VkMemoryRequirements memRequirements;
        // vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

        // VkMemoryAllocateInfo allocInfo{};
        // allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        // allocInfo.allocationSize = memRequirements.size;
        // allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to allocate buffer memory!");
        // }

        // vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_graphicCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		// TracyVkCollect(tracyContext, commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicQueue);

        vkFreeCommandBuffers(m_device, m_graphicCommandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		{
			// TracyVkZone(tracyContext, commandBuffer, "transferBuffer");
			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		}

        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers() {
        m_graphicCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo graphicAllocInfo{};
        graphicAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        graphicAllocInfo.commandPool = m_graphicCommandPool;
        graphicAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        graphicAllocInfo.commandBufferCount = (uint32_t) m_graphicCommandBuffers.size();

        if (vkAllocateCommandBuffers(m_device, &graphicAllocInfo, m_graphicCommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo	computeAllocInfo{};
        computeAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        computeAllocInfo.commandPool = m_computeCommandPool;
        computeAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        computeAllocInfo.commandBufferCount = (uint32_t) m_computeCommandBuffers.size();

        if (vkAllocateCommandBuffers(m_device, &computeAllocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        VkCommandBufferAllocateInfo tracyAllocInfo{};
        tracyAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        tracyAllocInfo.commandPool = m_graphicCommandPool;
        tracyAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        tracyAllocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_device, &tracyAllocInfo, &tracyCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
        
    }

	void renderSnowflake(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Snowflake");

		Object object = Object::SNOWFLAKE;
		tinygltf::Model& model = m_model[object];
		auto& attributes = model.meshes[0].primitives[0].attributes;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.snowflake);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent.width;
		viewport.height = (float) swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer instanceBuffer{m_storageBuffers.snowflake[m_currentFrame].buffer};
		uint32_t instanceCount{SNOWFLAKE_COUNT};
		size_t positionBufferOffset = model.accessors[attributes["POSITION"]].byteOffset;

		VkBuffer vertexBuffers[2] = {m_vertexBuffers.snowflake.buffer, instanceBuffer};
		VkDeviceSize vertexBufferOffsets[2] = {positionBufferOffset, 0};
		vkCmdBindVertexBuffers(commandBuffer, 0, sizeof(vertexBuffers) / sizeof(VkBuffer), vertexBuffers, vertexBufferOffsets);

		auto& indexAccessoridx = model.meshes[0].primitives[0].indices;
		VkBuffer indexBuffer = m_indexBuffers.snowflake.buffer;
		uint64_t indexBufferOffsets = model.accessors[indexAccessoridx].byteOffset;
		uint64_t verticesCount = model.accessors[indexAccessoridx].count;

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, indexBufferOffsets, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.snowflake, 
					   0, 1, &m_graphicDescriptorSets.snowflake[m_currentFrame], 0, 0);

		vkCmdDrawIndexed(commandBuffer, verticesCount, instanceCount, 0, 0, 0);
	}

	void renderCandles(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Candles");
		Object object = Object::CANDLE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent.width;
		viewport.height = (float) swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		m_vkCmdSetPrimitiveTopologyEXT(commandBuffer, DynamicPrimitiveTopologies[s_currentTopologyIdx]);

		tinygltf::Model& model = m_model[object];

		int meshIdx = 0;
		// factor out tangent
		auto& attribute = model.meshes[0].primitives[0].attributes;
		uint32_t instanceCount = m_towerInstanceRaw.size(); 
		
		for (auto& mesh : model.meshes) {
			bool isAnimated = m_vertexBuffers.candles[meshIdx].size() == 4 ? true : false;

			if (isAnimated) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.candles.separated);

				// assume there is 1 primitive per mesh
				std::vector<VkBuffer> buffers;
				std::vector<VkDeviceSize> bufferOffsets;
				auto& atrributes = mesh.primitives[0].attributes;

				// some mesh of the model don't have tangent attribute
				// WARNING: tangent attribute will get a random buffer as dummy
				const SpvReflectShaderModule& reflection = m_shaders.candlesVS.reflection;
				for(unsigned int i = 0; i < reflection.input_variable_count - 1 /*exclude instance buffer*/; i++) {
				unsigned int bufferIdx{0};
				std::string reflectionAttr = getNameAttrAtIndex(reflection, i);
					for(unsigned int j = 0; j < atrributes.size(); j++) {
						auto modelAttrIt = atrributes.begin();
						std::advance(modelAttrIt, j);
						if (modelAttrIt->first == AttrNameMap[reflectionAttr])	
							bufferIdx = j;
					}
					VkBuffer buffer = m_vertexBuffers.candles[meshIdx][bufferIdx].buffer;
					size_t bufferOffset = 0;
					buffers.push_back(buffer);
					bufferOffsets.push_back(bufferOffset);
				}

				buffers.push_back(m_towerInstanceBuffer);
				bufferOffsets.push_back(0);

				vkCmdBindVertexBuffers(commandBuffer, 0, buffers.size(), buffers.data(), bufferOffsets.data());
			}
			else {
				assert(m_vertexBuffers.candles[meshIdx].size() == 1);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.candles.interleaved);
				std::array<VkBuffer, 2> buffers = {m_vertexBuffers.candles[meshIdx][0].buffer, m_towerInstanceBuffer};
				std::array<VkDeviceSize, 2> offsets = {0, 0};
				vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers.data(), offsets.data());

			}

			auto& indexAccessoridx = mesh.primitives[0].indices;
			unsigned int idxCount{0};
			if(m_indexBuffers.candles.lod1[meshIdx].size == 0 || !useLOD) {
				VkBuffer indexBuffer = m_indexBuffers.candles.lod0[meshIdx].buffer;
				uint64_t indexBufferOffsets = 0;
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer, indexBufferOffsets, VK_INDEX_TYPE_UINT32);
				idxCount = model.accessors[indexAccessoridx].count;
			}
			else {
				VkBuffer indexBuffer = m_indexBuffers.candles.lod1[meshIdx].buffer;
				uint64_t indexBufferOffsets = 0;
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer, indexBufferOffsets, VK_INDEX_TYPE_UINT32);
				idxCount = m_indexBuffers.candles.lod1[meshIdx].size / sizeof(unsigned int);
			}

			// WARNING: remove this
			// some meshes have animation and don't normal map
			if(isAnimated) {
				m_graphicPushConstant.candles.value = 0;
			}
			else {
				m_graphicPushConstant.candles.value = 1;
			}

			vkCmdPushConstants(commandBuffer, m_graphicPipelineLayouts.candles, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Float), (void*)&m_graphicPushConstant.candles);

			uint32_t DynamicOffset{};
			// this dynamic offset have to be 256 byte aligned
			DynamicOffset = sizeof(CandlesPerMeshTransform) * meshIdx;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.candles, 
						   0, 1, &m_graphicDescriptorSets.candles.tranformUniform[m_currentFrame], 1, &DynamicOffset);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.candles, 
						   1, 1, &m_graphicDescriptorSets.candles.meshMaterial[meshIdx][m_currentFrame], 0, 0);

			// is this count right?
			vkCmdDrawIndexed(commandBuffer, idxCount, instanceCount, 0, 0, 0);
			meshIdx++;
		}
	}

	void renderFloor(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render floor");

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.floor);

		VkBuffer vertexBuffers[1] = {m_vertexBuffers.quad.buffer};
		VkDeviceSize vertexBufferOffsets[1] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, sizeof(vertexBuffers) / sizeof(VkBuffer), vertexBuffers, vertexBufferOffsets);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.floor, 
					   0, 1, &m_graphicDescriptorSets.floor[m_currentFrame], 0, 0);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void renderSkybox(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render skybox");

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.skybox);

		VkBuffer vertexBuffers[1] = {m_vertexBuffers.cube.buffer};
		VkDeviceSize vertexBufferOffsets[1] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, sizeof(vertexBuffers) / sizeof(VkBuffer), vertexBuffers, vertexBufferOffsets);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.skybox, 
					   0, 1, &m_graphicDescriptorSets.skybox[m_currentFrame], 0, 0);

		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}

	void renderShadowMap(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Shadow Map");

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.shadow.directional);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_shadowExtent.width;
		viewport.height = (float)m_shadowExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = m_shadowExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkDeviceSize vertexBufferOffsets[1] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffers.shadow.buffer, vertexBufferOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffers.shadow.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.shadow, 
					   0, 1, &m_graphicDescriptorSets.shadow.directional[m_currentFrame], 0, 0);

		vkCmdDrawIndexed(commandBuffer, m_indexBuffers.shadow.size / sizeof(unsigned int), m_sceneContext.candlesShadowInstanceCount, 0, 0, 0);
	}

	void renderShadowView(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.shadow.viewport);

		VkViewport viewport{};
		viewport.x = (float)swapChainExtent.width * 3 / 4;
		viewport.y = (float)swapChainExtent.height * 3 / 4;
		viewport.width = (float)swapChainExtent.width / 4;
		viewport.height = (float)swapChainExtent.height / 4;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkDeviceSize vertexBufferOffsets[1] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffers.quad.buffer, vertexBufferOffsets);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.bloom, 
					   0, 1, &m_graphicDescriptorSets.shadow.viewport[m_currentFrame], 0, 0);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void renderBloomHorizontal(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Bloom Horizontal");
		VkDeviceSize offsets{0};

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.bloom.horizontal);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffers.quad.buffer, &offsets);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.bloom,
						0, 1, &m_graphicDescriptorSets.bloom1[m_currentFrame], 0, 0);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void renderBloomVertical(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Bloom Vertical");
		VkDeviceSize offsets{0};

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.bloom.vertical);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffers.quad.buffer, &offsets);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.bloom,
						0, 1, &m_graphicDescriptorSets.bloom2[m_currentFrame], 0, 0);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void renderCombine(VkCommandBuffer commandBuffer) {
		TracyVkZone(tracyContext, commandBuffer, "Render Combine Pass");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelines.combine);

		VkDeviceSize offsets{0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffers.quad.buffer, &offsets);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipelineLayouts.combine,
						0, 1, &m_graphicDescriptorSets.combine[m_currentFrame], 0, 0);

		vkCmdPushConstants(commandBuffer, m_graphicPipelineLayouts.candles, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Float), (void*)&m_graphicPushConstant.combine);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	void releaseTransientBuffersAtCmdIdx(int idx) {
		std::vector<Buffer>& bufferToRelease = m_transientBuffers[idx];
		for(auto& buffer : bufferToRelease) {
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
			vmaFreeMemory(m_allocator, buffer.allocation);
		}
		bufferToRelease.clear();
	}
	
	void transferAnimVertexBuffers(VkCommandBuffer commandBuffer) {
		Object objIdx = Object::CANDLE;

		std::vector<VkBufferMemoryBarrier> animBarriers{};
		for (unsigned int meshIdx = 0; meshIdx < m_model[objIdx].meshes.size(); meshIdx++) {
			for (unsigned int attrIdx = 0; attrIdx < m_vertexBuffers.candles[meshIdx].size(); attrIdx++) {
				if (m_vertexBuffers.candles[meshIdx][attrIdx].needTransfer == false || m_vertexBuffers.candles[meshIdx][attrIdx].size == 0)
					continue;

				// Transfer vertex position animation data
				VkBuffer stagingBuffer;
				VmaAllocation stagingAlloc;
				unsigned int size = m_vertexBuffers.candles[meshIdx][attrIdx].size;

				createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAlloc);

				void* data;
				vmaMapMemory(m_allocator, stagingAlloc, &data);
					memcpy(data, m_vertexBuffers.candles[meshIdx][attrIdx].raw, static_cast<size_t>(size));
				vmaUnmapMemory(m_allocator, stagingAlloc);

				VkBufferCopy copyRegion{};
				copyRegion.size = size;

				vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_vertexBuffers.candles[meshIdx][attrIdx].buffer, 1, &copyRegion);

				Buffer transientBuffer;
				transientBuffer.buffer = stagingBuffer;
				transientBuffer.allocation = stagingAlloc;
				m_transientBuffers[m_currentFrame].push_back(std::move(transientBuffer));

				VkBufferMemoryBarrier animBarrier{};
				animBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				animBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; 
				animBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				animBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				animBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				animBarrier.buffer = m_vertexBuffers.candles[meshIdx][attrIdx].buffer;
				animBarrier.size = size;
				animBarrier.offset = 0;
			 
				animBarriers.push_back(animBarrier);
			}
		}

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			0, nullptr,
			animBarriers.size(), animBarriers.data(),
			0, nullptr);
	}

	void transferLod1IndexBuffers(VkCommandBuffer commandBuffer) {
		std::vector<VkBufferMemoryBarrier> lod1Barriers{};
		std::vector<VkBuffer> stagingBuffers{};
		std::vector<VmaAllocation> stagingAllocs{};
		for (auto& buffer : m_indexBuffers.candles.lod1) {
			if (buffer.needTransfer == false || buffer.size == 0)
				continue;

			Buffer newBuffer{};
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAloc{};

			// WARNING: new buffer size could be bigger than existing vulkan buffer
			createBuffer(buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferAloc);
			void* data;
			vmaMapMemory(m_allocator, stagingBufferAloc, &data);
				memcpy(data, buffer.raw, buffer.size);
			vmaUnmapMemory(m_allocator, stagingBufferAloc);

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = buffer.size;

			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer.buffer, 1, &copyRegion);

			buffer.needTransfer = false;

			Buffer transientBuffer;
			transientBuffer.buffer = stagingBuffer;
			transientBuffer.allocation = stagingBufferAloc;
			m_transientBuffers[m_currentFrame].push_back(std::move(transientBuffer));

			VkBufferMemoryBarrier lod1Barrier{};
			lod1Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			lod1Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; 
			lod1Barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			lod1Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			lod1Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			lod1Barrier.buffer = buffer.buffer;
			lod1Barrier.size = buffer.size;
			lod1Barrier.offset = 0;
		 
			lod1Barriers.push_back(lod1Barrier);
		}

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			0, nullptr,
			lod1Barriers.size(), lod1Barriers.data(),
			0, nullptr);
	}

	void transferFrameBuffers(VkCommandBuffer commandBuffer) {
		transferAnimVertexBuffers(commandBuffer);
		transferLod1IndexBuffers(commandBuffer);
	}

    void recordGraphicCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

		{
			ZoneScopedN("Wait Transfer Animation Buffers");
			TracyVkZone(tracyContext, commandBuffer, "Transfer animation buffers");
			transferFrameBuffers(commandBuffer);
		}

		{
			// shadow
			VkRenderPassBeginInfo shadowPassInfo{};
			shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			shadowPassInfo.renderPass = m_renderPasses.shadow;
			shadowPassInfo.framebuffer = m_frameBuffers.shadow[m_currentFrame];
			shadowPassInfo.renderArea.offset = {0, 0};
			shadowPassInfo.renderArea.extent = m_shadowExtent;

			VkClearValue shadowClear{};
			shadowClear.depthStencil.depth = 1;
			shadowClear.depthStencil.stencil = 0;
			shadowPassInfo.clearValueCount = 1;
			shadowPassInfo.pClearValues = &shadowClear;

			vkCmdBeginRenderPass(commandBuffer, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				renderShadowMap(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);

			// base
			VkRenderPassBeginInfo basePassInfo{};
			basePassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			basePassInfo.renderPass = m_renderPasses.base;
			basePassInfo.framebuffer = m_frameBuffers.base[m_currentFrame];
			basePassInfo.renderArea.offset = {0, 0};
			basePassInfo.renderArea.extent = swapChainExtent;

			std::array<VkClearValue, 5> clearValues{};
			clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clearValues[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clearValues[2].depthStencil = {1.0f, 0};
			clearValues[3].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clearValues[4].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

			basePassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			basePassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &basePassInfo, VK_SUBPASS_CONTENTS_INLINE);

				renderSnowflake(commandBuffer);
				renderCandles(commandBuffer);
				renderFloor(commandBuffer);
				renderSkybox(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);

			// bloom
			VkRenderPassBeginInfo bloomPassInfo{};
			bloomPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bloomPassInfo.renderPass = m_renderPasses.bloom;
			bloomPassInfo.framebuffer = m_frameBuffers.bloom.horizontal[m_currentFrame];
			bloomPassInfo.renderArea.offset = {0, 0};
			bloomPassInfo.renderArea.extent = swapChainExtent;

			std::array<VkClearValue, 1> bloomClearValues{};
			bloomClearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

			bloomPassInfo.clearValueCount = static_cast<uint32_t>(bloomClearValues.size());
			bloomPassInfo.pClearValues = bloomClearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &bloomPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				renderBloomHorizontal(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);

			bloomPassInfo.framebuffer = m_frameBuffers.bloom.vertical[m_currentFrame];
			vkCmdBeginRenderPass(commandBuffer, &bloomPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				renderBloomVertical(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);

			// combine
			VkRenderPassBeginInfo combinePassInfo{};
			combinePassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			combinePassInfo.renderPass = m_renderPasses.combine;
			combinePassInfo.framebuffer = m_frameBuffers.combine[imageIndex];
			combinePassInfo.renderArea.offset = {0, 0};
			combinePassInfo.renderArea.extent = swapChainExtent;

			std::array<VkClearValue, 1> combineClearValues{};
			combineClearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

			combinePassInfo.clearValueCount = static_cast<uint32_t>(combineClearValues.size());
			combinePassInfo.pClearValues = combineClearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &combinePassInfo, VK_SUBPASS_CONTENTS_INLINE);

				renderCombine(commandBuffer);
				renderShadowView(commandBuffer);

				{
					TracyVkZone(tracyContext, commandBuffer, "Draw ImGui");
					ImGui::Render();
					ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, VK_NULL_HANDLE);
				}

			vkCmdEndRenderPass(commandBuffer);
		}
		TracyVkCollect(tracyContext, commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record graphic command buffer!");
        }
    }

	void recordComputeCommandBuffer(VkCommandBuffer commandBuffer){
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
            throw std::runtime_error("failed to begin recording command buffer!");
		}

		{
			// uint32_t lastFrame = (m_currentFrame - 1) % MAX_FRAMES_IN_FLIGHT;

			// VkBufferMemoryBarrier lastSnowflakeStorageBarrier{};
			// lastSnowflakeStorageBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			// lastSnowflakeStorageBarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT; 
			// lastSnowflakeStorageBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			// lastSnowflakeStorageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			// lastSnowflakeStorageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			// lastSnowflakeStorageBarrier.buffer = m_storageBuffers.snowflake[lastFrame].buffer;
			// lastSnowflakeStorageBarrier.size = VK_WHOLE_SIZE;
			// lastSnowflakeStorageBarrier.offset = 0;

			// vkCmdPipelineBarrier(commandBuffer,
			// 	VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
			// 	0, nullptr,
			// 	1, &lastSnowflakeStorageBarrier,
			// 	0, nullptr);
		}

		{
			TracyVkZone(tracyContext, commandBuffer, "Dispatch Snowflake Compute");
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSets.snowflake[m_currentFrame], 0, nullptr);
			vkCmdPushConstants(commandBuffer, m_computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstant), (void*)&m_computePushConstant);
			// FIXME: choose right number of workgroups
			vkCmdDispatch(commandBuffer, 1024, 1, 1);
		}

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record compute command buffer!");
        }
	}

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_computeStartingSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightGraphicFences.resize(MAX_FRAMES_IN_FLIGHT);

		m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightComputeFences.resize(MAX_FRAMES_IN_FLIGHT);

		// we can submit an empty command buffer to signal the m_renderFinishedSemaphores but
		// doing this way creating a timeline semaphore is cooler
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		// VkSemaphoreTypeCreateInfo semaphoreTypeInfo{};
		// semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		// semaphoreTypeInfo.initialValue = 1;
		// semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        // semaphoreInfo.pNext = &semaphoreTypeInfo;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_computeStartingSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightGraphicFences[i]) != VK_SUCCESS ){
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }

			CHECK_VK_RESULT(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightComputeFences[i])
						, "fail to create Compute fence");

			CHECK_VK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i])
					  , "failed to create compute synchronization objects for a frame!");
        }

		// signal the last index computeStartingSemaphore because if we don't do manually, noone do :(
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_computeStartingSemaphores[MAX_FRAMES_IN_FLIGHT - 1];
		CHECK_VK_RESULT(vkQueueSubmit(m_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE)
				  ,"fail to submit semaphore signaling to queue");
		CHECK_VK_RESULT(vkQueueWaitIdle(m_graphicQueue)
				  ,"fail to wait for semaphore signling queuing");
    }


	void processImGui(){
		if(ImGui::CollapsingHeader("Keybindings")) {
			ImGui::SeparatorText("Control");
			ImGui::BulletText("'C' : Cursor control only - lock camera");
			ImGui::BulletText("'X' : Camera control only - lock cursor");
			ImGui::BulletText("'ASDW' : movement");
			ImGui::BulletText("'Space - Shift' : move up - down");

			ImGui::SeparatorText("Render");
			ImGui::BulletText("'L' : all LOD1");
			ImGui::BulletText("'K' : re-produce LOD1 applying changes");
			ImGui::BulletText("'P' : change topology");
			ImGui::BulletText("'H' : toggle HDR");
			ImGui::BulletText("'R' : re-create pipelines");
		}
		
		ImGui::Spacing();
        ImGui::SeparatorText("Time");
			ImGui::Text("Current time: (%f)", m_lastTime);
			ImGui::Text("Delta time: (%f)", m_currentDeltaTime);
			ImGui::Text("FPS: (%f)", 1 / m_currentDeltaTime);

		ImGui::Spacing();
        ImGui::SeparatorText("Geometry");
		ImGui::SliderFloat("LOD1 generating target error", &s_targetError, 0.f, 1.f, "%.05f");
		ImGui::SliderFloat2("Texture attribute weights", &s_attrWeights[0], 0.f, 1.f, "%.05f");
		ImGui::SliderFloat3("Normal attribute weights", &s_attrWeights[2], 0.f, 1.f, "%.05f");
		ImGui::SliderFloat4("Tangent attribute weights", &s_attrWeights[5], 0.f, 1.f, "%.05f");

		ImGui::Spacing();
        ImGui::SeparatorText("Transform");
			ImGui::Text("Camera front: (%f), (%f), (%f)", g_camera.getFront().x, g_camera.getFront().y, g_camera.getFront().z);
			ImGui::Text("Camera position: (%f), (%f), (%f)", g_camera.getPostion().x, g_camera.getPostion().y, g_camera.getPostion().z);

			ImGui::SliderFloat("Far Plane", &s_farPlane, -10.f, 100.f, "%.5f");

			if(ImGui::CollapsingHeader("Objects")) {
				ImGui::SeparatorText("Snowflake Model");
				ImGui::SliderFloat3("Translate", s_snowTranslate, -10.f, 10.f, "%.2f");
				ImGui::SliderFloat3("Rotate", s_snowRotate, -10.f, 10.f, "%.2f");
				ImGui::SliderFloat3("Scale", s_snowScale, -10.f, 10.f, "%.2f");
			}

		ImGui::Spacing();
		ImGui::SeparatorText("Lighting");
			ImGui::SliderFloat3("Light Direction", (float*)&s_lightDir.x, -20.f, 20.f, "%.2f");
			if(ImGui::CollapsingHeader("Shadow Frustum")) {
				ImGui::SliderFloat("Shadow Far Plane", &s_shadowFarPlane, -10.f, 100.f, "%.5f");
				ImGui::SliderFloat("Shadow Left Plane", &s_shadowLeftPlane, -50.f, 50.f, "%.5f");
				ImGui::SliderFloat("Shadow Right Plane", &s_shadowRightPlane, -50.f, 50.f, "%.5f");
				ImGui::SliderFloat("Shadow Bot Plane", &s_shadowBotPlane, -50.f, 50.f, "%.5f");
				ImGui::SliderFloat("Shadow Top Plane", &s_shadowTopPlane, -50.f, 50.f, "%.5f");
			}

		ImGui::Spacing();
		ImGui::SeparatorText("Effect");
			if(ImGui::CollapsingHeader("HDR")) {
				ImGui::SliderFloat("Exposure", &m_graphicPushConstant.combine.value, 0.f, 1.f, "%.05f");
			}
	}

    void drawFrame() {
		ZoneScopedN("Render");

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// ImGui::ShowDemoWindow();
		processImGui();

		uint32_t imageIndex{};
		VkResult result{};
		{
			ZoneScopedN("Submit Compute Command Buffer");
			{
				ZoneScopedN("Wait for previous Compute Fence");
				vkWaitForFences(m_device, 1, &m_inFlightComputeFences[m_currentFrame], VK_TRUE, UINT64_MAX);
				vkResetFences(m_device, 1, &m_inFlightComputeFences[m_currentFrame]);
			}
			{
				// NOTE: only update Uniform buffer after the command buffer with the same m_currentFrame (the last 2 frames) have FINISHED.
				// have to update uniform after WaitForFence or else uniform are override within that frame
				updateComputeUniformBuffer();
				updateComputePushConstant();

				ZoneScopedN("Dispatch Compute Command Buffer");
				vkResetCommandBuffer(m_computeCommandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
				recordComputeCommandBuffer(m_computeCommandBuffers[m_currentFrame]);

				VkSubmitInfo computeSubmitInfo{};
				VkSemaphore* computeWaitSemaphores = nullptr;
				VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
				VkSemaphore computeSignalSemaphores[] = {m_computeFinishedSemaphores[m_currentFrame]};
				computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				// WARNING: is this optimal ?
				// computeSubmitInfo.waitSemaphoreCount = sizeof(computeWaitSemaphores) / sizeof(VkSemaphore);
				computeSubmitInfo.waitSemaphoreCount = 0;
				computeSubmitInfo.pWaitSemaphores = computeWaitSemaphores;
				computeSubmitInfo.pWaitDstStageMask = waitStages;
				computeSubmitInfo.signalSemaphoreCount = sizeof(computeSignalSemaphores) / sizeof(VkSemaphore);
				computeSubmitInfo.pSignalSemaphores = computeSignalSemaphores;
				computeSubmitInfo.commandBufferCount = 1;
				computeSubmitInfo.pCommandBuffers = &m_computeCommandBuffers[m_currentFrame];

				CHECK_VK_RESULT(vkQueueSubmit(m_computeQueue, 1, &computeSubmitInfo, m_inFlightComputeFences[m_currentFrame])
					, "fail to submit compute command buffer");
				// vkQueueWaitIdle(m_computeQueue);
			}
		}

		{
			ZoneScopedN("Submit Graphic Command Buffer");
			{
				ZoneScopedN("Wait for Graphic Fence");
				vkWaitForFences(m_device, 1, &m_inFlightGraphicFences[m_currentFrame], VK_TRUE, UINT64_MAX);
				vkResetFences(m_device, 1, &m_inFlightGraphicFences[m_currentFrame]);
			}
			{
				// NOTE: have to wait on m_inFlightGraphicFences before accquiring the next image because m_imageAvailableSemaphores may have NOT been un-signed
				ZoneScopedN("Accquire Next Image");
				result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					recreateSwapChain();
					return;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
					throw std::runtime_error("failed to acquire swap chain image!");
				}
				// std::cout << "imageIndex: " << imageIndex << "\n";
			}

			// NOTE: only update Uniform buffer after the command buffer with the same m_currentFrame (the last 2 frames) have FINISHED.
			// have to update uniform after WaitForFence or else uniform are override within that frame
			updateGraphicUniformBuffer();

			releaseTransientBuffersAtCmdIdx(m_currentFrame);
			vkResetCommandBuffer(m_graphicCommandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
			recordGraphicCommandBuffer(m_graphicCommandBuffers[m_currentFrame], imageIndex);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame], m_computeFinishedSemaphores[m_currentFrame]};
			// waitStage have to be TOP_OF_PIPELINE because there are resources (RT, descriptor set for transform) that change per frame
			// COLOR_ATTACHMENT_OUTPUT can result in this frame use the resouces of 2 frame ago (if MAX_FRAME_IN_FLIGHT = 2)
			VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
			submitInfo.waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(VkSemaphore);
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_graphicCommandBuffers[m_currentFrame];

			VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
			submitInfo.signalSemaphoreCount = sizeof(signalSemaphores) / sizeof(VkSemaphore);
			submitInfo.pSignalSemaphores = signalSemaphores;

			if (VkResult res = vkQueueSubmit(m_graphicQueue, 1, &submitInfo, m_inFlightGraphicFences[m_currentFrame])) {
				std::string msg = "failed to submit graphic command buffer with CODE: " + vk::to_string((vk::Result)res);
				throw std::runtime_error(msg);
			}
		}	

		{
			ZoneScopedN("Submit present image");
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

			VkSwapchainKHR swapChains[] = {m_swapChain};
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
				framebufferResized = false;
				recreateSwapChain();
			} else if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}
		}

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }




	std::string getNameAttrAtIndex(SpvReflectShaderModule module, uint8_t idx) {
		for (unsigned int i = 0; i < module.input_variable_count; i++) {
			if(module.input_variables[i]->location == idx)
				return module.input_variables[i]->name;
		}
		return "";
	}
