#include <unordered_map>
#include <string>
#include "vulkan/vulkan.h"
#include "allocator.hpp"

#include "cgltf/cgltf.h"

struct IMaterial {
	std::string name;
	std::string fragmentShader;
};

struct StandardMaterial : IMaterial {
	std::unordered_map<std::string, cgltf_material> materialMeta;
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
