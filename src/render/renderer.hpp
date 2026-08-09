#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <vector>

#include "Tracy.hpp"
#include "TracyVulkan.hpp"
#include "allocator.hpp"
#include "camera.hpp"
#include "cgltf/cgltf.h"
#include "global.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "platform.hpp"
#include "shader.hpp"
#include "util.hpp"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"

#ifdef ENABLE_OPTIMIZE_MESH
#include "meshoptimizer.h"
#endif  // ENABLE_OPTIMIZE_MESH

static uint32_t s_currentTopologyIdx{0};
static bool useLOD{false};
static const float c_overdrawThreshold{1.05f};
static bool s_isLodUpdated{false};
static float s_targetError{0.5f};
// 3 NORMAL - 4 TANGENT - 2 TEXCOORD_0
static float s_attrWeights[9] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

inline std::array<VkPrimitiveTopology, 3> DynamicPrimitiveTopologies{
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN};

struct Float {
    alignas(4) float value{1};
};

struct Int {
    alignas(4) int value{0};
};

struct SpecializationConstant {
    alignas(4) int useTexture{1};
} s_specConstant;

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

struct Buffer {
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
          needTransfer(false) {}
};

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

// Transform sceen data to vulkan object/calls
class Renderer {
   public:
    void init();

    void loadModels();
    void loadShaders();
    void initVertexData();
    void computeAnimation();
    void initIndexData();
    // analyzeMeshes(false);
    void optimizeMeshes();
    void generateIndexLOD();
    void initShadowData();
    void initSceneContext();
    void initUniformData();
    // analyzeMeshes(true);
    void loadInstanceData();

    void createCommandBuffers();
    void createSyncObjects();
    void createSwapchainImageViews();
    void createDescriptorSetLayouts();
    void createPipelineCache();
    void createPipelineLayouts();
    void createPipelines();
    void createCommandPools();
    void createModelImages();
    void createSkyboxImage();
    void createRenderTargets();
    void createSamplers();
    void createVertexBuffers();
    void createIndexBuffers();
    void createInstanceBuffer();
    void createUniformBuffers();
    void createStorageBuffer();
    void createDescriptorPool();
    void createDescriptorSets();

    std::vector<float> computeWeights(cgltf_data* i_model, unsigned int meshIdx);
    std::vector<float> interleaveAttributes(cgltf_mesh* i_mesh);
    void computeMorphTargets(cgltf_data* i_model, unsigned int i_meshIdx,
                             std::vector<float> i_weights);

   private:
    Device* m_device;
    Platform* m_platform;
    Allocator m_allocator;

    ShaderManager m_shaderManager;
    tsl::robin_map<std::string, cgltf_data*> m_models;
    tsl::robin_map<std::string, std::vector<glm::mat4>> m_modelMeshTransforms;

    std::vector<VkImageView> m_swapChainImageViews;

    // WARNING: Sus, need to factor these out
    struct {
        GfxBuffer snowflake;
        GfxBuffer quad;
        GfxBuffer cube;
        GfxBuffer shadow;
        // some meshes have one interleave buffer, some have each attribute as 1 buffer
        std::vector<std::vector<GfxBuffer>> candles;
    } m_vertexBuffers;

    struct {
        GfxBuffer snowflake;
        GfxBuffer quad;
        GfxBuffer shadow;
        // candle model have multiple meshes each have 2 lod
        struct {
            std::vector<GfxBuffer> lod0;
            std::vector<GfxBuffer> lod1;
        } candles;
    } m_indexBuffers;

    struct {
        std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> snowflake;
        struct {
            std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> perMeshTransform;
            std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> lightingTransform;
        } candles;
        std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> floor;
        std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> skybox;
        struct {
            std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> perMeshTransform;
            GfxBuffer perInstanceTransform;
            GfxBuffer lightTransform;
        } shadow;
    } m_graphicUniformBuffers;

    struct VertexInstance {
        alignas(16) glm::vec3 pos;
    };
    std::vector<VertexInstance> m_towerInstanceRaw;
    VkBuffer m_towerInstanceBuffer;
    VmaAllocation instanceBufferAlloc;

    struct {
        std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> snowflake;
    } m_storageBuffers;

    struct {
        struct {
            std::array<GfxBuffer, MAX_FRAMES_IN_FLIGHT> vortex;
        } snowflake;
    } m_computeUniformBuffers;

    float m_lastTime;
    float m_currentDeltaTime = 0;
    float m_currentAnimTime = 0;

    uint32_t m_currentFrame = 0;

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

    Camera m_camera;
};
