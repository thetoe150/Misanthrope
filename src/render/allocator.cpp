#include "allocator.hpp"

Allocator::Allocator(vk::Device i_device, VmaAllocator i_allocator) {
    m_device = i_device;
    m_allocator = i_allocator;
}

Allocator::~Allocator() {
    vmaDestroyAllocator(m_allocator);
}

void printMemoryBudget() {}

void printMemoryStatistics() {}

vk::Buffer Allocator::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                   vk::MemoryPropertyFlags properties, VmaAllocation& allocation) {
    vk::Buffer buffer{};

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo{};
    if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocInfo.priority = 0.0f;
    } else {
        // allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
        // VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
        allocInfo.priority = 1.0f;
    }

    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = 0;
    allocInfo.preferredFlags = (VkMemoryPropertyFlags)properties;
    allocInfo.memoryTypeBits = 0;
    allocInfo.pool = VK_NULL_HANDLE;
    allocInfo.pUserData = nullptr;

    if (vmaCreateBuffer(m_allocator, (VkBufferCreateInfo*)&bufferInfo, &allocInfo,
                        (VkBuffer*)&buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    return buffer;
}

vk::Image Allocator::createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                                 vk::SampleCountFlagBits numSamples, vk::Format format,
                                 vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                 vk::MemoryPropertyFlags properties, VmaAllocation& imageAlloc) {
    vk::Image image{};

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    // imageAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    imageAllocInfo.priority = 1.0f;

    vmaCreateImage(m_allocator, (VkImageCreateInfo*)&imageInfo, &imageAllocInfo, (VkImage*)&image,
                   &imageAlloc, nullptr);

    return image;
}

GpuBuffer Allocator::allocateUnifiedBuffer(CpuBuffer i_cpuBuffer, BufferType i_type) {
    vk::BufferUsageFlags usage{};
    if (i_type == BufferType::UNIFORM_BUFFER) {
        usage |= vk::BufferUsageFlagBits::eUniformBuffer;
    }
    GpuBuffer buffer;
    buffer.size = i_cpuBuffer.size;
    buffer.buffer = createBuffer(i_cpuBuffer.size, usage,
                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent |
                                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 buffer.allocation);

    vmaMapMemory(m_allocator, buffer.allocation, &i_cpuBuffer.data);
    return buffer;
}

GpuBuffer Allocator::allocateDeviceBuffer(CpuBuffer i_cpuBuffer, BufferType i_type) {
    assert(i_cpuBuffer.size != 0);

    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst;
    if (i_type == BufferType::VERTEX_BUFFER) {
        usage |= vk::BufferUsageFlagBits::eVertexBuffer;
    } else if (i_type == BufferType::INDEX_BUFFER) {
        usage |= vk::BufferUsageFlagBits::eIndexBuffer;
    }

    GpuBuffer buffer{};
    buffer.buffer = createBuffer(i_cpuBuffer.size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 buffer.allocation);
    buffer.size = i_cpuBuffer.size;

    return buffer;
}

int Allocator::map(GfxBuffer i_gfxBuffer) {
    assert(i_gfxBuffer.cpuBuffer.data != nullptr);
    assert(i_gfxBuffer.cpuBuffer.size == i_gfxBuffer.gpuBuffer.size);

    StagingBuffer stageBuffer;
    stageBuffer.buffer = createBuffer(
        i_gfxBuffer.cpuBuffer.size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stageBuffer.allocation);

    void* data;
    vmaMapMemory(m_allocator, stageBuffer.allocation, &data);
    memcpy(data, i_gfxBuffer.cpuBuffer.data, i_gfxBuffer.cpuBuffer.size);
    vmaUnmapMemory(m_allocator, stageBuffer.allocation);

    assignBufferTransfer(stageBuffer.buffer, i_gfxBuffer.gpuBuffer.buffer,
                         i_gfxBuffer.cpuBuffer.size);

    m_stageBuffers.push_back(stageBuffer);

    return 1;
}

void Allocator::assignImageTransfer(vk::Buffer i_buffer, vk::Image i_image, uint32_t i_width,
                                    uint32_t i_height) {
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{i_width, i_height, 1};

    ImageTransferJob transfer{};
    transfer.srcBuffer = i_buffer;
    transfer.dstImage = i_image;
    transfer.copy = region;

    m_imageTransferJobs.push_back(transfer);
}

void Allocator::assignBufferTransfer(vk::Buffer i_srcBuffer, vk::Buffer i_dstBuffer,
                                     vk::DeviceSize size) {
    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    BufferTransferJob transfer;
    transfer.srcBuffer = i_srcBuffer;
    transfer.dstBuffer = i_dstBuffer;
    transfer.copy = copyRegion;

    m_bufferTransferJobs.push_back(transfer);
}

void Allocator::submitTransferJob(vk::CommandBuffer i_commandBuffer) {
    for (auto& job : m_bufferTransferJobs) {
        i_commandBuffer.copyBuffer(job.srcBuffer, job.dstBuffer, 1, &job.copy);
    }

    for (auto& job : m_imageTransferJobs) {
        i_commandBuffer.copyBufferToImage(job.srcBuffer, job.dstImage,
                                          vk::ImageLayout::eTransferDstOptimal, 1, &job.copy);
    }
}

Allocator createAllocator(const Device* i_device) {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                          VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorInfo.physicalDevice = i_device->m_physicalDevice;
    allocatorInfo.device = i_device->m_device;
    allocatorInfo.preferredLargeHeapBlockSize = 0;
    allocatorInfo.pAllocationCallbacks = nullptr;
    allocatorInfo.pDeviceMemoryCallbacks = nullptr;
    allocatorInfo.pHeapSizeLimit = nullptr;
    allocatorInfo.pVulkanFunctions = nullptr;
    allocatorInfo.instance = i_device->m_instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator VmaAllocator{};
    if (vmaCreateAllocator(&allocatorInfo, &VmaAllocator) != VK_SUCCESS) {
        throw std::runtime_error("fail to create memory allocator");
    }

    Allocator allocator(i_device->m_device, VmaAllocator);
    return allocator;
}
