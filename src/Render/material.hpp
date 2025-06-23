#include <unordered_map>
#include <string>
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "cgltf/cgltf.h"

struct IMaterial {
	std::string name;
	std::string fragmentShader;
};

struct StandardMaterial : IMaterial {
	std::unordered_map<std::string, cgltf_texture> textures;   // Texture slots by name
	std::unordered_map<std::string, float> floatParams;   // Float uniforms (e.g., metallic, roughness)
};

struct ShadowMaterial : IMaterial {
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

class MaterialLoader {

};
