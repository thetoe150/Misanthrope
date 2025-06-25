#include "shader.hpp"

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void ShaderLoader::LoadShader() {
	loadShader(m_shaders.snowflakeVS, "../../src/shaders/snowflake.vert.spv");
	loadShader(m_shaders.snowflakeFS, "../../src/shaders/snowflake.frag.spv");
	loadShader(m_shaders.snowflakeCS, "../../src/shaders/snowflake.comp.spv");

	loadShader(m_shaders.candlesVS, "../../src/shaders/candles.vert.spv");
	loadShader(m_shaders.candlesFS, "../../src/shaders/candles.frag.spv");

	loadShader(m_shaders.skyboxVS, "../../src/shaders/skybox.vert.spv");
	loadShader(m_shaders.skyboxFS, "../../src/shaders/skybox.frag.spv");

	loadShader(m_shaders.floorVS, "../../src/shaders/floor.vert.spv");
	loadShader(m_shaders.floorFS, "../../src/shaders/floor.frag.spv");

	loadShader(m_shaders.quadVS, "../../src/shaders/quad.vert.spv");
	loadShader(m_shaders.bloomFS, "../../src/shaders/bloom.frag.spv");
	loadShader(m_shaders.combineFS, "../../src/shaders/combine.frag.spv");
	loadShader(m_shaders.shadowViewportFS, "../../src/shaders/shadow_viewport.frag.spv");

	loadShader(m_shaders.shadowBatchVS, "../../src/shaders/shadow_batch.vert.spv");

	// for (unsigned int i = 0; i < m_shaders.candlesVS.reflection.input_variable_count; i++) {
	// 	std::cout << m_shaders.candlesVS.reflection.input_variables[i]->name << " : " << m_shaders.candlesVS.reflection.input_variables[i]->location << " - ";
	// }
	// std::cout << m_shaders.candlesVS.reflection.input_variable_count << " total attributes \n";

	// for (unsigned int i = 0; i < m_shaders.candlesVS.reflection.output_variable_count; i++) {
	// 	std::cout << m_shaders.snowflakeVS.reflection.output_variables[i]->name << " - ";
	// }
	// std::cout << m_shaders.snowflakeVS.reflection.output_variable_count << " total output attribute \n";

	AttrNameMap["a_position"] = "POSITION";
	AttrNameMap["a_normal"] = "NORMAL";
	AttrNameMap["a_tangent"] = "TANGENT";
	AttrNameMap["a_texCoord"] = "TEXCOORD_0";
	// AttrNameMap["instancePos"] = "POSITION";
};
