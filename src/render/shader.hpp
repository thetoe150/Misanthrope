#ifndef SHADER_H
#define SHADER_H

#include <memory>
#include <stdexcept>
#include <vector>

#include "SPIRV-Reflect/spirv_reflect.h"
#include "device.hpp"
#include "robin_map/robin_map.h"
#include "util.hpp"
#include "vulkan/vulkan.hpp"

#define MAX_DESCRIPTOR_SET 4
#define MAX_BINDING 5
#define MAX_LOCALTION 20
#define MAX_PUSH_CONSTANT 3
#define MAX_BUFFER 8
#define MAX_MEMBER 8

// There is some assumption for Shader Resource
// - only use 2 descriptor set per drawcall, first ds is for data not changing through a frame,
//		second is for data changing per drawcall.
// - There is no storage buffer descriptor. Buffer device addressing is used and passing through
// push constant.
// - All the textures is stored in the last binding of each descriptor set (descriptor indexing).
enum class Type {
    I = 0,
    F,
    F2,
    F3,
    F4,
    F3x3,
    F4x4,

    COUNT
};

enum class Stage : uint8_t {
    VERTEX = (1 << 0),
    FRAGMENT = (1 << 1),

    COUNT
};

enum class Semantic : uint8_t {
    POSITION,
    NORMAL,
    TANGENT,
    COLOR,
    TEXCOORD_0,
    TEXCOORD_1,
    TEXCOORD_2,

    COUNT
};

enum class BindingType : uint8_t {
    UNIFORM,
    DYNAMIC_UNIFORM,
    TEXTURE_SAMPLER,
    TEXTURE_SAMPLER_ARRAY,

    COUNT,
};

struct Location {
    std::string name;
    Semantic semantic;
    Type type;
    uint8_t location;
    bool isInput;
};

struct Binding {
    std::string name;
    BindingType type{BindingType::COUNT};
    Stage stage;

    union BindingDesc {
        int8_t blockIdx;
        int8_t samplerIdx{-1};
    };
    BindingDesc descIdx;
};

struct DescriptorSet {
    uint8_t bindingCount{0};
    Binding bindings[MAX_BINDING];
};

struct BlockMember {
    std::string name;
    Type type;
    uint8_t offset;
};

struct Block {
    uint8_t memberCount{0};
    BlockMember members[MAX_MEMBER];
    uint32_t arraySize;
    uint8_t stride;
};

struct PushConstant {
    std::string name;
    Block block;
    uint8_t size;
    Stage stage;
};

struct Sampler {};

struct Reflection {
    uint8_t locationCount{0};
    Location locations[MAX_LOCALTION];

    std::array<DescriptorSet, 2> descriptorSets;
    uint8_t totalBindingCount{0};

    uint8_t blockCount{0};
    Block blocks[MAX_BUFFER];

    uint8_t samplerCount{0};
    Sampler samplers[MAX_BUFFER];

    uint8_t pushConstantCount{0};
    PushConstant pushConstants[MAX_PUSH_CONSTANT];
};

struct Shader {
    vk::Device device;
    vk::ShaderModule module;
    SpvReflectShaderModule reflection;
};

class ShaderManager {
   public:
    ShaderManager();
    ~ShaderManager();
    void loadShaders(const std::vector<std::string>& i_paths);
    Shader createShader(const std::string& i_path);
    vk::ShaderModule createShaderModule(const std::vector<uint8_t>& i_blob);

    Shader getShader(const std::string& i_paths);
    tsl::robin_map<std::string, std::string> AttrNameMap;

   private:
    vk::Device m_device;
    tsl::robin_map<std::string, Shader> m_shaders;
};

#endif  // SHADER_H
