#include "layout.hpp"

std::size_t LayoutManager::computeDescriptorSetLayoutHash(const Reflection& i_reflection) {
    std::size_t h = std::hash<uint8_t>{}(i_reflection.locationCount);
    for (const auto& set : i_reflection.descriptorSets) {
        hash_combine(h, (uint8_t)set.bindingCount);
        for (unsigned int i = 0; i < set.bindingCount; i++) {
            hash_combine(h, (uint8_t)set.bindings[i].type);
            hash_combine(h, (uint8_t)set.bindings[i].stage);
        }
    }

    return h;
}

std::array<vk::DescriptorSetLayout, 2> LayoutManager::createDescriptorSetLayouts(
    const Reflection& i_reflection) {
    assert(i_reflection.descriptorSets.size() == 2);
    std::array<vk::DescriptorSetLayout, 2> layouts{};
    for (unsigned int setIdx = 0; setIdx < i_reflection.descriptorSets.size(); setIdx++) {
        const DescriptorSet& set = i_reflection.descriptorSets[setIdx];
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (unsigned int bindIdx = 0; bindIdx < set.bindingCount; bindIdx++) {
            vk::DescriptorSetLayoutBinding binding{};
            binding.binding = bindIdx;
            // descriptor indexing is used so descriptorCount is set when allocating descriptor.
            binding.descriptorCount = 1;
            binding.descriptorType = getVkTypeFromReflectType(set.bindings[bindIdx].type);
            binding.stageFlags = getVkStageFromReflectStage(set.bindings[bindIdx].stage);
            binding.pImmutableSamplers = nullptr;

            bindings.emplace_back(binding);
        }

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (m_device.createDescriptorSetLayout(&layoutInfo, nullptr, &layouts[setIdx]) !=
            vk::Result::eSuccess) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    return layouts;
}

std::array<vk::DescriptorSetLayout, 2> LayoutManager::getDescriptorSetLayouts(
    const Reflection& i_reflection) {
    std::size_t h = computeDescriptorSetLayoutHash(i_reflection);
    if (!m_descriptorSetLayouts.contains(h)) {
        auto descriptorSetLayouts = createDescriptorSetLayouts(i_reflection);
        m_descriptorSetLayouts.insert(std::pair(h, descriptorSetLayouts));
    }
    return m_descriptorSetLayouts[h];
}

std::size_t LayoutManager::computePipelineLayoutHash(const Reflection& i_reflection) {
    std::size_t h = computeDescriptorSetLayoutHash(i_reflection);
    hash_combine(h, (uint8_t)i_reflection.pushConstantCount);
    for (const auto& pc : i_reflection.pushConstants) {
        hash_combine(h, (uint8_t)pc.size);
        hash_combine(h, (uint8_t)pc.stage);
    }

    return h;
}

vk::PipelineLayout LayoutManager::createPipelineLayouts(const Reflection& i_reflection) {
    std::vector<vk::PushConstantRange> pushRanges{};

    for (unsigned int i = 0; i < i_reflection.pushConstantCount; i++) {
        vk::PushConstantRange pushConstant{};
        pushConstant.size = i_reflection.pushConstants[i].size;
        pushConstant.stageFlags = getVkStageFromReflectStage(i_reflection.pushConstants[i].stage);
        pushConstant.offset = 0;
    }

    std::array<vk::DescriptorSetLayout, 2> desLayouts = getDescriptorSetLayouts(i_reflection);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = desLayouts.size();
    pipelineLayoutInfo.pSetLayouts = desLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushRanges.data();

    vk::PipelineLayout pipelineLayout;
    if (m_device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("failed to create graphic pipeline layout!");
    }

    return pipelineLayout;
}

vk::PipelineLayout LayoutManager::getPipelineLayouts(const Reflection& i_reflection) {
    std::size_t h = computePipelineLayoutHash(i_reflection);
    if (!m_pipelineLayouts.contains(h)) {
        auto PipelineLayouts = createPipelineLayouts(i_reflection);
        m_pipelineLayouts.insert(std::pair(h, PipelineLayouts));
    }
    return m_pipelineLayouts[h];
}

vk::DescriptorType getVkTypeFromReflectType(BindingType i_type) {
    switch (i_type) {
        case BindingType::UNIFORM:
            return vk::DescriptorType::eUniformBuffer;

        case BindingType::DYNAMIC_UNIFORM:
            return vk::DescriptorType::eUniformBufferDynamic;

        case BindingType::TEXTURE_SAMPLER:
        case BindingType::TEXTURE_SAMPLER_ARRAY:
            return vk::DescriptorType::eCombinedImageSampler;

        default:
            std::cout << "invalid binding!!!\n";
    }
}

vk::ShaderStageFlags getVkStageFromReflectStage(Stage i_type) {
    vk::ShaderStageFlags flags{};
    if (static_cast<uint8_t>(i_type) & static_cast<uint8_t>(Stage::VERTEX)) {
        flags |= vk::ShaderStageFlagBits::eVertex;
    }
    if (static_cast<uint8_t>(i_type) & static_cast<uint8_t>(Stage::FRAGMENT)) {
        flags |= vk::ShaderStageFlagBits::eFragment;
    }

    return flags;
}
