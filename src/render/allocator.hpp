#ifndef ALLOCATOR_H_INCLUDED
#define ALLOCATOR_H_INCLUDED

#include <assert.h>

#include <memory>

#include "device.hpp"
#include "vulkan/vulkan.hpp"

enum BufferType { VERTEX_BUFFER, INDEX_BUFFER, UNIFORM_BUFFER };

struct GpuBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
    uint32_t size;
};

struct CpuBuffer {
    void* data;
    uint32_t size;
};

struct GfxBuffer {
    CpuBuffer cpuBuffer;
    GpuBuffer gpuBuffer;
};

struct ImageTransferJob {
    vk::BufferImageCopy copy{};
    vk::Buffer srcBuffer;
    vk::Image dstImage;
};

struct BufferTransferJob {
    vk::BufferCopy copy{};
    vk::Buffer srcBuffer;
    vk::Buffer dstBuffer;
};

struct StagingBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
};

class Allocator {
   public:
    Allocator(vk::Device i_device, VmaAllocator i_allocator);
    ~Allocator();

    GpuBuffer allocateDeviceBuffer(CpuBuffer i_cpuBuffer, BufferType i_type);
    GpuBuffer allocateUnifiedBuffer(CpuBuffer i_cpuBuffer, BufferType i_type);

    void submitTransferJob(vk::CommandBuffer i_commandBuffer);
    int map(GfxBuffer i_gfxBuffer);

   private:
    vk::Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                          vk::SampleCountFlagBits numSamples, vk::Format format,
                          vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties, VmaAllocation& imageAlloc);

    vk::Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                            vk::MemoryPropertyFlags properties, VmaAllocation& allocation);
    void assignImageTransfer(vk::Buffer i_buffer, vk::Image i_image, uint32_t i_width,
                             uint32_t i_height);
    void assignBufferTransfer(vk::Buffer i_srcBuffer, vk::Buffer i_dstBuffer, vk::DeviceSize size);

    vk::Device m_device;
    VmaAllocator m_allocator;
    std::vector<ImageTransferJob> m_imageTransferJobs;
    std::vector<BufferTransferJob> m_bufferTransferJobs;
    std::vector<StagingBuffer> m_stageBuffers;
};

Allocator createAllocator(const Device* i_device);

#endif /* #ifndef ALLOCATOR_H_INCLUDED */
