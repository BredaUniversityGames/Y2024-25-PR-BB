#include "shader_reflector.hpp"

#include "vulkan_helper.hpp"

ShaderReflector::ShaderReflector(const VulkanBrain& brain)
    : _brain(brain)
{
    _inputAssemblyStateCreateInfo = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };

    _multisampleStateCreateInfo = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    _viewportStateCreateInfo = {
        .viewportCount = 1,
        .scissorCount = 1,
    };

    _rasterizationStateCreateInfo = {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f,
    };
}

ShaderReflector::~ShaderReflector()
{
    for (auto& shaderStage : _shaderStages)
    {
        spvReflectDestroyShaderModule(&shaderStage.reflectModule);
    }
}

void ShaderReflector::AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes, std::string_view entryPoint)
{
    SpvReflectShaderModule reflectModule;
    util::VK_ASSERT(spvReflectCreateShaderModule(spirvBytes.size(), spirvBytes.data(), &reflectModule),
        "Failed reflecting on shader module!");

    _shaderStages.emplace_back(ShaderStage {
        .stage = stage,
        .entryPoint = entryPoint,
        .spirvBytes = spirvBytes,
        .reflectModule = reflectModule });
}

vk::Pipeline ShaderReflector::BuildPipeline()
{
    ReflectShaders();
    CreatePipelineLayout();
    return CreatePipeline();
}

void ShaderReflector::ReflectShaders()
{
    for (const auto& shaderStage : _shaderStages)
    {
        ReflectVertexInput(shaderStage);
        ReflectPushConstants(shaderStage);
        ReflectDescriptorLayouts(shaderStage);

        vk::ShaderModule module = CreateShaderModule(shaderStage.spirvBytes);
        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
            .stage = shaderStage.stage,
            .module = module,
            .pName = shaderStage.entryPoint.data(),
        };
        _pipelineShaderStages.emplace_back(pipelineShaderStageCreateInfo);
    }
}

void ShaderReflector::ReflectVertexInput(const ShaderStage& shaderStage)
{
    uint32_t inputCount { 0 };

    spvReflectEnumerateInputVariables(&shaderStage.reflectModule, &inputCount, nullptr);
    std::vector<SpvReflectInterfaceVariable*> inputVariables { inputCount };
    spvReflectEnumerateInputVariables(&shaderStage.reflectModule, &inputCount, inputVariables.data());

    uint32_t binding { 0 };
    vk::VertexInputBindingDescription bindingDescription {
        .binding = binding,
        .stride = 0,
        .inputRate = vk::VertexInputRate::eVertex
    };

    for (const auto* var : inputVariables)
    {
        if (var->location == SPV_REFLECT_VARIABLE_FLAGS_UNUSED)
            continue;

        vk::VertexInputAttributeDescription attributeDescription {
            .location = var->location,
            .binding = binding,
            .format = static_cast<vk::Format>(var->format),
            .offset = bindingDescription.stride
        };

        bindingDescription.stride += util::FormatSize(attributeDescription.format);

        _attributeDescriptions.emplace_back(attributeDescription);
    }

    _bindingDescriptions.emplace_back(bindingDescription);
}

void ShaderReflector::ReflectPushConstants(const ShaderReflector::ShaderStage& shaderStage)
{
    uint32_t pushCount { 0 };
    spvReflectEnumeratePushConstantBlocks(&shaderStage.reflectModule, &pushCount, nullptr);
    std::vector<SpvReflectBlockVariable*> pushConstants { pushCount };
    spvReflectEnumeratePushConstantBlocks(&shaderStage.reflectModule, &pushCount, pushConstants.data());

    for (const auto& pushConstant : pushConstants)
    {
        vk::PushConstantRange pushRange {
            .stageFlags = shaderStage.stage,
            .offset = pushConstant->offset,
            .size = pushConstant->size,
        };

        _pushConstantRanges.emplace_back(pushRange);
    }
}

void ShaderReflector::ReflectDescriptorLayouts(const ShaderReflector::ShaderStage& shaderStage)
{
    uint32_t setCount { 0 };
    spvReflectEnumerateDescriptorSets(&shaderStage.reflectModule, &setCount, nullptr);
    std::vector<SpvReflectDescriptorSet*> sets { setCount };
    spvReflectEnumerateDescriptorSets(&shaderStage.reflectModule, &setCount, sets.data());

    for (const auto& set : sets)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        for (size_t i = 0; i < set->binding_count; ++i)
        {
            const SpvReflectDescriptorBinding* reflectBinding = set->bindings[i];
            vk::DescriptorSetLayoutBinding binding {
                .binding = reflectBinding->binding,
                .descriptorType = static_cast<vk::DescriptorType>(reflectBinding->descriptor_type),
                .descriptorCount = reflectBinding->count,
                .stageFlags = shaderStage.stage,
                .pImmutableSamplers = nullptr,
            };

            bindings.emplace_back(binding);
        }

        vk::DescriptorSetLayoutCreateInfo layoutInfo {
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        _descriptorSetLayouts.emplace_back(_brain.device.createDescriptorSetLayout(layoutInfo, nullptr));
    }
}

void ShaderReflector::CreatePipelineLayout()
{
    vk::PipelineLayoutCreateInfo createInfo {
        .setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size()),
        .pSetLayouts = _descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size()),
        .pPushConstantRanges = _pushConstantRanges.data(),
    };

    _pipelineLayout = _brain.device.createPipelineLayout(createInfo, nullptr);
}

vk::Pipeline ShaderReflector::CreatePipeline()
{
    if (!_inputAssemblyStateCreateInfo.has_value() || !_viewportStateCreateInfo.has_value() || !_rasterizationStateCreateInfo.has_value() || !_multisampleStateCreateInfo.has_value() || !_depthStencilStateCreateInfo.has_value() || !_colorBlendStateCreateInfo.has_value())
    {
        throw std::runtime_error("Failed creating pipeline, missing information1");
    }

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .vertexBindingDescriptionCount = static_cast<uint32_t>(_bindingDescriptions.size()),
        .pVertexBindingDescriptions = _bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(_attributeDescriptions.size()),
        .pVertexAttributeDescriptions = _attributeDescriptions.data(),
    };

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .stageCount = static_cast<uint32_t>(_pipelineShaderStages.size()),
        .pStages = _pipelineShaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &_inputAssemblyStateCreateInfo.value(),
        .pViewportState = &_viewportStateCreateInfo.value(),
        .pRasterizationState = &_rasterizationStateCreateInfo.value(),
        .pMultisampleState = &_multisampleStateCreateInfo.value(),
        .pDepthStencilState = &_depthStencilStateCreateInfo.value(),
        .pColorBlendState = &_colorBlendStateCreateInfo.value(),
        .layout = _pipelineLayout,
        .renderPass = nullptr,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    vk::PipelineRenderingCreateInfoKHR renderingCreateInfo {
        .colorAttachmentCount = static_cast<uint32_t>(_colorAttachmentFormats.size()),
        .pColorAttachmentFormats = _colorAttachmentFormats.data(),
        .depthAttachmentFormat = _depthFormat,
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;
    structureChain.assign(graphicsPipelineCreateInfo);
    structureChain.assign(renderingCreateInfo);

    auto [result, pipeline] = _brain.device.createGraphicsPipeline(nullptr, structureChain.get(), nullptr);

    util::VK_ASSERT(result, "Failed creating graphics pipeline!");

    return pipeline;
}

vk::ShaderModule ShaderReflector::CreateShaderModule(const std::vector<std::byte>& spirvBytes)
{
    vk::ShaderModuleCreateInfo createInfo {
        .codeSize = spirvBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(spirvBytes.data()),
    };

    return _brain.device.createShaderModule(createInfo, nullptr);
}