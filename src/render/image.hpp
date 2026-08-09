#include "allocation.hpp"
#include "vulkan/vulkan.hpp"

vk::ImageView createImageView(vk::Device i_deivce, vk::Image image, vk::Format format,
                              vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

typedef struct {
    vk::Image image;
    VmaAllocation allocation;
    vk::ImageView view;
    ImageType type;
} Image;

typedef struct {
    std::vector<Image> image;
} MeshImages;
