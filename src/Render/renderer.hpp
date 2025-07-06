#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"

#include "allocator.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Tracy.hpp"
#include "TracyVulkan.hpp"

#include <vector>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <math.h>
#include <assert.h>

#include "platform.hpp"

// 3 NORMAL - 4 TANGENT - 2 TEXCOORD_0
static float s_attrWeights[9] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

inline std::array<VkPrimitiveTopology, 3> DynamicPrimitiveTopologies{
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
};

class Drawcall {
	Slot vertexBuffer;
	Slot indexBuffer;
	std::vector<VkImageView> images;
};

class Renderer {
public:

private:
	vk::Device m_device;
};
