#ifndef MESH_H
#define MESH_H

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "SPIRV-Reflect/spirv_reflect.h"
#include "allocator.hpp"
#include "cgltf/cgltf.h"
#include "glm/glm.hpp"
#include "material.hpp"
#include "meshoptimizer.h"
#include "robin_map/robin_map.h"
#include "util.hpp"

// manage mesh data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class Mesh {
   public:
    struct MeshDesc {
        std::string name;
        uint32_t primitiveCount;
        uint32_t instanceCount;
        bool isAnimated;
        Material material;
        uint32_t lodCount;
        uint32_t bindingCount;
    };
    virtual void acquireGpuResource();
    virtual void gatherPipelineDesc();

    virtual void initVertexData();
    virtual void createVertexBuffer();
    virtual void initIndexData();
    virtual void createIndexBuffer();
    virtual void createUniformBuffer();

   protected:
    cgltf_data* m_model;
    cgltf_mesh* m_mesh;
    MeshDesc m_meshDesc;
    Allocator* m_gpuAllocator;
    MemoryView m_instanceData;
    glm::mat4 m_transform;
};

class StaticMesh : Mesh {
   public:
    void acquireGpuResource() override;
    void gatherPipelineDesc() override;
    void initVertexData() override;
    void createVertexBuffer() override;
    void initIndexData() override;
    void createIndexBuffer() override;
    void createUniformBuffer() override;

   private:
    std::shared_ptr<cgltf_mesh> m_meshMeta;

    GfxBuffer m_vertexBuffer;
    std::vector<GfxBuffer> m_indexBuffers;
};

class AnimatedMesh : Mesh {
   public:
    void acquireGpuResource() override;
    void gatherPipelineDesc() override;
    void initVertexData() override;
    void createVertexBuffer() override;
    void initIndexData() override;
    void createIndexBuffer() override;
    void createUniformBuffer() override;

    void traverseModelNodesForTransform(const cgltf_node* node, glm::mat4 mat);
    std::vector<float> computeFrameWeights(unsigned int meshIdx, float deltaTime);
    void computeFrameMorphTargets(unsigned int meshIdx, std::vector<float> weights);

   private:
    tsl::robin_map<std::string, GfxBuffer> m_vertexBuffers;
    GfxBuffer m_positionBuffer;

    unsigned int m_currentAnimTime;
    unsigned int m_currentDeltaTime;
};

class BatchedMesh : Mesh {
   public:
    void acquireGpuResource() override;

   private:
    std::vector<std::shared_ptr<cgltf_mesh>> meshMetas;
    MemoryView mesh;
};

// load meshes from files and provide them for models do processing
class MeshLoader {
   public:
    MeshLoader();
    uint8_t loadMeshes(std::vector<cgltf_data*>);
    cgltf_mesh ProvideMesh(std::string);

   private:
    tsl::robin_map<std::string, std::shared_ptr<cgltf_mesh>> meshMetas;
    tsl::robin_map<std::string, std::shared_ptr<MemoryView>> meshes;
};

struct VertexInstance {
    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 4;
        bindingDescription.stride = sizeof(VertexInstance);
        bindingDescription.inputRate = vk::VertexInputRate::eInstance;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};
        attributeDescriptions[0].binding = 4;
        attributeDescriptions[0].location = 4;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        // attributeDescriptions[0].offset = offsetof(VertexInstance, pos);
        attributeDescriptions[0].offset = 0;

        return attributeDescriptions;
    }
};

void traverseModelNodesForTransform(std::string obj, const cgltf_node* node, glm::mat4 mat);
cgltf_accessor* getAccessorForAttr(cgltf_primitive& i_primitive, cgltf_attribute_type i_type);
cgltf_accessor* getAccessorForAttr(cgltf_morph_target& i_primitive, cgltf_attribute_type i_type);
int getIndexForAttr(cgltf_primitive& i_primitive, cgltf_attribute_type i_type);
std::string getNameAttrAtIndex(const SpvReflectShaderModule* module, uint8_t idx);
cgltf_attribute_type getModelAttributeForShaderAttribute(std::string i_attributeName);
unsigned char* getBufferPointerFromAccessor(const cgltf_accessor* i_accessor);
float* allocateFloatBufferForAccessor(const cgltf_accessor* i_accessor);

#endif  // MESH_H
