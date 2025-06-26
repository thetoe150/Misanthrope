#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"

#include "allocator.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Tracy.hpp"
#include "TracyVulkan.hpp"

#include <vector>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <math.h>
#include <assert.h>

#include "platform.hpp"

enum Object{
	CANDLE = 0,
	SNOWFLAKE,
	COUNT
};

static std::array<const char*, 6> cubeBoxImageFiles{
	"../../data/skyboxes/IceLake/posx.jpg",
	"../../data/skyboxes/IceLake/negx.jpg",
	"../../data/skyboxes/IceLake/posy.jpg",
	"../../data/skyboxes/IceLake/negy.jpg",
	"../../data/skyboxes/IceLake/posz.jpg",
	"../../data/skyboxes/IceLake/negz.jpg"
};

struct Vortex {
	alignas(16) glm::vec3 pos;
	alignas(4) float force;
	alignas(4) float radius;
	alignas(4) float height;
};

inline auto getVortexRadius = [](float currentValue, float delta) -> float {
	return 2 + 3 * std::sin(delta);
};

inline auto getVortexVelocity = [](float currentValue, float delta) -> float {
	return 2 + 3 * std::sin(delta);
};

struct Snowflake {
	glm::vec3 position;
	float weight;
};

struct ComputePushConstant{
	int snowflakeCount = SNOWFLAKE_COUNT;
	float deltaTime;
};

struct Float{
	alignas(4) float value{1};
};

struct Int{
	alignas(4) int value{0};
};

struct SpecializationConstant{
	alignas(4) int useTexture{1};
}s_specConstant;

struct SnowTransform {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 viewProj;
};

struct CandlesPerMeshTransform {
    alignas(64) glm::mat4 model;
	// required alignment for descriptor uniform buffer offset
    alignas(64) glm::mat4 dummy1;
    alignas(64) glm::vec3 dummy2;
    alignas(64) glm::vec3 dummy3;
};

struct CandlesLightingTransform {
    alignas(16) glm::mat4 viewProj;
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 camPos;
};

struct FloorTransform {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 camViewProj;
    alignas(16) glm::mat4 lightViewProj;
    alignas(16) glm::vec3 camPos;
    alignas(16) glm::vec3 lightPos;
};

struct SkyboxTransform {
    alignas(16) glm::mat4 camView;
    alignas(16) glm::mat4 camProj;
};

struct ShadowLightingTransform {
    alignas(16) glm::mat4 viewProj;
};

struct ShadowPerMeshTransform {
    alignas(16) glm::mat4 model;
};

static uint32_t s_currentTopologyIdx{0};
static bool useLOD{false};
static const float c_overdrawThreshold{1.05f};
static bool s_isLodUpdated{false};
static float s_targetError{0.5f};
// 3 NORMAL - 4 TANGENT - 2 TEXCOORD_0
static float s_attrWeights[9] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

inline std::array<VkPrimitiveTopology, 3> DynamicPrimitiveTopologies{
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
};

float generateRandomFloat(float low, float high){
	return low + (static_cast<float>(rand()) / (RAND_MAX / (high - low)));
}

class Drawcall {
	Slot vertexBuffer;
	Slot indexBuffer;
	std::vector<VkImageView> images;
};

class Renderer {
private:
	tracy::VkCtx* tracyContext;
	std::shared_ptr<VkDevice> m_device;
	VmaAllocator m_allocator;
    std::vector<VkImageView> m_swapChainImageViews;


	struct {
		std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> base;
		std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> shadow;
		struct {
			std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> horizontal;
			std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> vertical;
		} bloom;
		std::vector<VkFramebuffer> combine;
	} m_frameBuffers;

	struct {
		VkRenderPass base;
		VkRenderPass shadow;
		VkRenderPass bloom;
		VkRenderPass combine;
	} m_renderPasses;

	struct {
		VkDescriptorSetLayout snowflake;
		struct {
			VkDescriptorSetLayout tranformUniform;
			VkDescriptorSetLayout meshMaterial;
		} candles;
		VkDescriptorSetLayout floor;
		VkDescriptorSetLayout skybox;
		VkDescriptorSetLayout shadow;
		VkDescriptorSetLayout bloom;
		VkDescriptorSetLayout combine;
	} m_graphicDescriptorSetLayouts;

	struct {
		VkPipelineLayout snowflake;
		VkPipelineLayout candles;
		VkPipelineLayout floor;
		VkPipelineLayout skybox;
		VkPipelineLayout shadow;
		VkPipelineLayout bloom;
		VkPipelineLayout combine;
	}
    m_graphicPipelineLayouts;

	struct {
		VkDescriptorSetLayout snowflake;
	} m_computeDescriptorSetLayouts;

    VkPipelineLayout m_computePipelineLayout;

	VkPipelineCache m_pipelineCache;
	std::vector<uint8_t> pipelineCacheBlob;

    VkPipeline m_computePipeline;
	struct {
		VkPipeline snowflake;
		struct {
			VkPipeline interleaved;
			VkPipeline separated;
		} candles;
		VkPipeline floor;
		VkPipeline skybox;
		struct {
			VkPipeline directional;
			VkPipeline viewport;
		} shadow;
		struct {
			VkPipeline vertical;
			VkPipeline horizontal;
		} bloom;
		VkPipeline combine;
	} m_graphicPipelines;

    VkCommandPool m_graphicCommandPool;
    VkCommandPool m_computeCommandPool;
	VkQueryPool timestampPool;

	struct {
		unsigned int candlesShadowMeshCount{0};
		unsigned int candlesShadowInstanceCount{0};

		glm::mat4 camView;
		glm::mat4 camProjection;
		glm::mat4 lightView;
		glm::mat4 lightProjection;

		glm::mat4 snowflakeModel;
		glm::mat4 candlesModel;
		std::vector<glm::mat4> candlesMeshesModels;
		std::vector<glm::mat4> candlesInstancePos;
		glm::mat4 floorModel;
		std::vector<glm::mat4> shadowBatchedModels;
		std::vector<glm::mat4> shadowInstanceModels;
	} m_sceneContext;

	std::map<Object, std::vector< glm::mat4>> m_modelMeshTransforms;

	std::map<Object, std::vector<std::vector< float>>> m_modelMeshFrameWeights;

	typedef struct {
		VkImage image;
		VmaAllocation allocation;
		VkImageView view;
	} Image;

	typedef struct {
		Image baseImage;
		Image normalImage;
		Image emissiveImage;
	} MeshImages;

	std::map<Object, std::vector<MeshImages>> m_modelImages;

	struct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	} m_skyboxImage;
	

	typedef struct {
		struct {
			Image colorRT;
			Image colorResRT;
			Image bloomThresholdRT;
			Image bloomThresholdResRT;
			Image depthRT;
		} base;
		Image shadow;
		Image bloom1;
		Image bloom2;
	} RenderTarget;

	std::array<RenderTarget, MAX_FRAMES_IN_FLIGHT> m_renderTargets;

    uint32_t mipLevels;

	struct {
		VkSampler candles;
		VkSampler shadow;
		VkSampler postFX;
		VkSampler skybox;
	}
	m_samplers;

	struct Buffer{
		void* raw;
		uint32_t size;
		VkBuffer buffer;
		VmaAllocation allocation;
		// set needTransfer to true only when raw and size won't match VkBuffer stored data and size
		bool needTransfer;

		Buffer()
		: size(0),
		  raw(nullptr),
		  buffer(VK_NULL_HANDLE),
		  allocation(VK_NULL_HANDLE),
		  needTransfer(false)
		{}
	};

	struct {
		Buffer snowflake;
		Buffer quad;
		Buffer cube;
		Buffer shadow;
		// some meshes have one interleave buffer, some have each attribute as 1 buffer
		std::vector<std::vector<Buffer>> candles;
	} m_vertexBuffers;

	struct {
		Buffer snowflake;
		Buffer quad;
		Buffer shadow;
		// candle model have multiple meshes each have 2 lod
		struct {
			std::vector<Buffer> lod0;
			std::vector<Buffer> lod1;
		} candles;
	} m_indexBuffers;

	struct {
		std::array<Buffer, MAX_FRAMES_IN_FLIGHT> snowflake;
		struct {
			std::array<Buffer, MAX_FRAMES_IN_FLIGHT> perMeshTransform;
			std::array<Buffer, MAX_FRAMES_IN_FLIGHT> lightingTransform;
		} candles;
		std::array<Buffer, MAX_FRAMES_IN_FLIGHT> floor;
		std::array<Buffer, MAX_FRAMES_IN_FLIGHT> skybox;
		struct {
			std::array<Buffer, MAX_FRAMES_IN_FLIGHT> perMeshTransform;
			Buffer perInstanceTransform;
			Buffer lightTransform;
		} shadow;
	} m_graphicUniformBuffers;

    std::vector<VertexInstance> m_towerInstanceRaw;
	VkBuffer m_towerInstanceBuffer;
	VmaAllocation instanceBufferAlloc;

	struct {
		std::array<Buffer, MAX_FRAMES_IN_FLIGHT> snowflake;
	} m_storageBuffers;

	struct {
		struct {
			std::array<Buffer, MAX_FRAMES_IN_FLIGHT> vortex;
		} snowflake;
	} m_computeUniformBuffers;


	std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> m_transientBuffers;

	SpecializationConstant m_graphicSpecConstant;
	struct {
		Int shadow;
		Float candles;
		Float combine{0.8};
	} m_graphicPushConstant;
	ComputePushConstant m_computePushConstant;

    VkDescriptorPool m_descriptorPool;

	struct {
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> snowflake;
		struct {
			std::vector<std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>> meshMaterial; // per mesh of candles model
			std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> tranformUniform; // 1 for candles model, update every frame
		} candles;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> floor;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> skybox;
		struct {
			std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> viewport;
			std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> directional;
		} shadow;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> bloom1;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> bloom2;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> combine;
	} m_graphicDescriptorSets;

    struct {
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> snowflake; 
	} m_computeDescriptorSets;

    std::vector<VkCommandBuffer> m_graphicCommandBuffers;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    VkCommandBuffer tracyCommandBuffer;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkSemaphore> m_computeStartingSemaphores;

    std::vector<VkFence> m_inFlightGraphicFences;
    std::vector<VkFence> m_inFlightComputeFences;
    std::vector<VkSemaphore> m_computeFinishedSemaphores;

	// ----------------------------- other ----------------------------------

	VkDescriptorPool imguiDescriptorPool;


    bool framebufferResized = false;
};
