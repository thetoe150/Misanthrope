#include "image.hpp"

vk::ImageView createImageView(vk::Device i_device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (i_device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create image view!");
	}

	return imageView;
}

void createSkyboxImage() {
	int x, y, n;
	unsigned char* firstImage = stbi_load(cubeBoxImageFiles[0], &x, &y, &n, STBI_rgb_alpha);
	VkDeviceSize imageSize = x * y * 4;
	std::cout << cubeBoxImageFiles[0] << " with imageSize: " << imageSize << "\n";

	unsigned char* cubeData = (unsigned char*)malloc(imageSize * 6);
	memcpy(cubeData, firstImage, imageSize);
	VkDeviceSize currentSize = imageSize;

	VkBuffer staggingBuffer;
	VkDeviceMemory staggingBufferMem;
	{
		for (unsigned int i = 1; i < 6; i++) {
			int x, y, n;
			unsigned char* image = stbi_load(cubeBoxImageFiles[i], &x, &y, &n, STBI_rgb_alpha);
			assert(x * y * 4 == imageSize);
			memcpy(cubeData + currentSize, image, imageSize);
			currentSize += imageSize;
			stbi_image_free(image);
		}

		VkBufferCreateInfo cubeImageBuffer{};
		cubeImageBuffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		cubeImageBuffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		cubeImageBuffer.size = imageSize * 6;
		cubeImageBuffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		CHECK_VK_RESULT(vkCreateBuffer(device, &cubeImageBuffer, 0, &staggingBuffer),
					  "Fail to create cube image buffer");

		VkMemoryRequirements memReq{};
		vkGetBufferMemoryRequirements(device, staggingBuffer, &memReq);
		std::cout << "mem req: " << (uint64_t)memReq.alignment;

		VkMemoryAllocateInfo staggingAllocInfo{};
		staggingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		assert(memReq.size == imageSize * 6);
		staggingAllocInfo.allocationSize = imageSize * 6;
		staggingAllocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		CHECK_VK_RESULT(vkAllocateMemory(device, &staggingAllocInfo, 0, &staggingBufferMem),
						"Fail to allocate memory for cube stagging buffer");

		vkBindBufferMemory(device, staggingBuffer, staggingBufferMem, 0);

		void* data;
		vkMapMemory(device, staggingBufferMem, 0, imageSize * 6, 0, &data);
			memcpy(data, cubeData, imageSize * 6);
		vkUnmapMemory(device, staggingBufferMem);

		free(cubeData);
	}

	VkDeviceMemory cubeImageMem;
	VkImage cubeImage;
	{
		VkImageCreateInfo cubeImageInfo{};
		cubeImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		cubeImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		cubeImageInfo.imageType = VK_IMAGE_TYPE_2D;

		cubeImageInfo.extent.width = x;
		cubeImageInfo.extent.height = y;
		cubeImageInfo.extent.depth = 1;
		cubeImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		cubeImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		cubeImageInfo.mipLevels = 1;
		cubeImageInfo.arrayLayers = 6;
		cubeImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		cubeImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		cubeImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		cubeImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		CHECK_VK_RESULT(vkCreateImage(device, &cubeImageInfo, 0, &cubeImage),
						"Fail to create cube image");

		VkMemoryRequirements memReq{};
		vkGetImageMemoryRequirements(device, cubeImage, &memReq);

		VkMemoryAllocateInfo imageMemInfo{};
		imageMemInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		assert(memReq.size == imageSize * 6);
		imageMemInfo.allocationSize = memReq.size;
		imageMemInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		CHECK_VK_RESULT(vkAllocateMemory(device, &imageMemInfo, 0, &cubeImageMem),
						"Fail to allocate memory for cube Image");
		vkBindImageMemory(device, cubeImage, cubeImageMem, 0);
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	{
		VkImageMemoryBarrier beginBarrier{};
		beginBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		beginBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		beginBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		beginBarrier.srcAccessMask = 0;
		beginBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		beginBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beginBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beginBarrier.image = cubeImage;
		beginBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		beginBarrier.subresourceRange.baseMipLevel = 0;
		beginBarrier.subresourceRange.levelCount = 1;
		beginBarrier.subresourceRange.baseArrayLayer = 0;
		beginBarrier.subresourceRange.layerCount = 6;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &beginBarrier);

		VkBufferImageCopy copy{};
		copy.imageExtent.width = x;
		copy.imageExtent.height = y;
		copy.imageExtent.depth = 1;
		copy.imageSubresource.mipLevel = 0;
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.layerCount = 6;

		vkCmdCopyBufferToImage(commandBuffer, staggingBuffer, cubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		VkImageMemoryBarrier endBarrier{};
		endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		endBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		beginBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		beginBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		endBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		endBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		endBarrier.image = cubeImage;
		endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		endBarrier.subresourceRange.baseMipLevel = 0;
		endBarrier.subresourceRange.levelCount = 1;
		endBarrier.subresourceRange.baseArrayLayer = 0;
		endBarrier.subresourceRange.layerCount = 6;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &endBarrier);
	}
	endSingleTimeCommands(commandBuffer);

	vkDestroyBuffer(device, staggingBuffer, 0);
	vkFreeMemory(device, staggingBufferMem, 0);
	m_skyboxImage.image = cubeImage;
	m_skyboxImage.memory = cubeImageMem;

	// Create Image view
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_skyboxImage.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	CHECK_VK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &m_skyboxImage.view), 
					"fail to create skybox view");

	// Create sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = m_m_physicalDeviceProperties.limits.maxSamplerAnisotropy; 

	CHECK_VK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &m_samplers.skybox),
				"fail to create skybox sampler");
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}
