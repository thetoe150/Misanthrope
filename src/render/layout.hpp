#include <glm/glm.hpp>

#include "shader.hpp"

struct SceneData {
    glm::mat4 camViewProj;
    glm::mat4 lightViewProj;
    glm::vec3 lightPos;
    glm::vec3 camPos;
};

struct SceneTexture {};

struct FrameShaderResource {
    SceneData sceneBuffers;
    SceneTexture sceneTextures;
};

struct DrawcallShaderResource {};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

vk::DescriptorType getVkTypeFromReflectType(BindingType i_type);
vk::ShaderStageFlags getVkStageFromReflectStage(Stage i_type);

class LayoutManager {
    std::size_t computeDescriptorSetLayoutHash(const Reflection& i_reflection);
    std::array<vk::DescriptorSetLayout, 2> createDescriptorSetLayouts(
        const Reflection& i_reflection);
    std::array<vk::DescriptorSetLayout, 2> getDescriptorSetLayouts(const Reflection& i_reflection);

    std::size_t computePipelineLayoutHash(const Reflection& i_reflection);
    vk::PipelineLayout createPipelineLayouts(const Reflection& i_reflection);
    vk::PipelineLayout getPipelineLayouts(const Reflection& i_reflection);

   private:
    vk::Device m_device;
    tsl::robin_map<std::size_t, std::array<vk::DescriptorSetLayout, 2>> m_descriptorSetLayouts;
    tsl::robin_map<std::size_t, vk::PipelineLayout> m_pipelineLayouts;
};
