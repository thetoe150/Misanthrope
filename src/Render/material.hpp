#ifndef MATERIAL_H 
#define MATERIAL_H 

#include <string>
#include "vulkan/vulkan.h"
#include "allocator.hpp"

#include "cgltf/cgltf.h"
#include "robin_map/robin_map.h"

// manage material data proccessed by model and create runtime vk objects, 1 mesh / drawcall
// info from this class is used to create drawcall
class IMaterial {
public:
private:
	std::string m_name;
	std::string m_fShader;
	std::vector<std::string> m_texture;
	cgltf_material materialMeta;
	bool m_isBlend;
};

class Material : IMaterial {
public:
private:
};

class ShadowMaterial : IMaterial {
public:
private:
};

typedef struct {
	vk::Image image;
	VmaAllocation allocation;
	vk::ImageView view;
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
