#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

#include "allocator.hpp"
#include "cgltf/cgltf.h"
#include "robin_map/robin_map.h"
#include "shader.hpp"
#include "vulkan/vulkan.h"

// manage material data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class Material {
   public:
   private:
    std::string m_name;
    std::string m_fragmentName;
    std::string m_vertexName;
    std::vector<std::string> m_texture;
    cgltf_material* materialMeta;
    bool m_isBlend;

    std::vector<vk::Image> m_images;
    Shader m_vertexShader;
    Shader m_fragmentShader;
};

class ShadowMaterial : Material {
   public:
   private:
};

enum class ImageType {
    Albedo,
    Normal,
    Emissive,
};

// load material from file and provide them for models do processing
class TextureLoader {
   public:
    TextureLoader();
};

#endif  // MATERIAL_H
