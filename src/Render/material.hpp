#ifndef MATERIAL_H 
#define MATERIAL_H 

#include <unordered_map>
#include <string>
#include "vulkan/vulkan.h"
#include "allocator.hpp"

#include "cgltf/cgltf.h"

// manage material data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class IMaterial {
public:
private:
	std::string name;
	std::string fragmentShader;
};

class StandardMaterial : IMaterial {
public:
private:
	std::unordered_map<std::string, cgltf_material> materialMeta;
};

class ShadowMaterial : IMaterial {
public:
private:
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

// load material from file and provide them for models do processing
class TextureLoader {
public:	
	TextureLoader();
};

#endif//MATERIAL_H 
