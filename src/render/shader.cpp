#include "shader.hpp"

vk::ShaderModule ShaderManager::createShaderModule(const std::vector<uint8_t>& code) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    vk::ShaderModule shaderModule;
    if (m_device.createShaderModule(&createInfo, nullptr, &shaderModule) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

Shader ShaderManager::createShader(const std::string& i_path) {
    Shader shader{};

    std::vector<uint8_t> blob = ReadFile(i_path);
    assert(!blob.empty());
    shader.module = createShaderModule(blob);
    SpvReflectResult result =
        spvReflectCreateShaderModule(blob.size(), blob.data(), &shader.reflection);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Copy elision (RVO and NRVO)
    // and fallback move semantic
    return shader;
}

void ShaderManager::loadShaders(const std::vector<std::string>& i_paths) {
    for (const auto& path : i_paths) {
        m_shaders.emplace(std::pair(path, createShader(path)));
    }

    // for (unsigned int i = 0; i < m_shaders.candlesVS.reflection.input_variable_count; i++) {
    // 	std::cout << m_shaders.candlesVS.reflection.input_variables[i]->name << " : " <<
    // m_shaders.candlesVS.reflection.input_variables[i]->location << " - ";
    // }
    // std::cout << m_shaders.candlesVS.reflection.input_variable_count << " total attributes \n";

    // // for (unsigned int i = 0; i < m_shaders.candlesVS.reflection.output_variable_count; i++) {
    // // 	std::cout << m_shaders.snowflakeVS.reflection.output_variables[i]->name << " - ";
    // // }
    // // std::cout << m_shaders.snowflakeVS.reflection.output_variable_count << " total output
    // attribute \n";

    // AttrNameMap["a_position"] = "POSITION";
    // AttrNameMap["a_normal"] = "NORMAL";
    // AttrNameMap["a_tangent"] = "TANGENT";
    // AttrNameMap["a_texCoord"] = "TEXCOORD_0";
    // // AttrNameMap["instancePos"] = "POSITION";
}

Shader ShaderManager::getShader(const std::string& i_path) {
    if (!m_shaders.contains(i_path)) {
        m_shaders.emplace(std::pair(i_path, createShader(i_path)));
    }
    return m_shaders[i_path];
}

ShaderManager::~ShaderManager() {
    m_shaders.clear();
}
