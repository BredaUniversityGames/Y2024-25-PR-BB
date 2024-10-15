#include "shader_reflector.hpp"

#include "vulkan_helper.hpp"

ShaderReflector::ShaderReflector(const VulkanBrain& brain)
    : _brain(brain)
{
}

ShaderReflector::~ShaderReflector()
{
    for (auto& shaderStage : _shaderStages)
    {
        spvReflectDestroyShaderModule(&shaderStage.reflectModule);
    }
}

void ShaderReflector::AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes)
{
    SpvReflectShaderModule reflectModule;
    util::VK_ASSERT(spvReflectCreateShaderModule(spirvBytes.size(), spirvBytes.data(), &reflectModule),
        "Failed reflecting on shader module!");

    _shaderStages.emplace_back(ShaderStage {
        .stage = stage,
        .entryPoint = "main",
        .spirvBytes = spirvBytes,
        .reflectModule = reflectModule });
}

vk::Pipeline ShaderReflector::BuildPipeline()
{
    ReflectShaders();
    CreateDescriptorLayouts();
    CreatePipelineLayout();
    return CreatePipeline();
}

void ShaderReflector::ReflectShaders()
{
    for (const auto& shaderStage : _shaderStages)
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

        vk::ShaderModule module = CreateShaderModule(shaderStage.spirvBytes);
        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
            .stage = shaderStage.stage,
            .module = module,
            .pName = shaderStage.entryPoint.c_str(),
        };
        _pipelineShaderStages.emplace_back(pipelineShaderStageCreateInfo);
    }
}

void ShaderReflector::CreateDescriptorLayouts()
{
    for (const auto& shaderStage : _shaderStages)
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
    // TODO: reflect on vertex data
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

    // TODO: provide more info
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
    };

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .stageCount = static_cast<uint32_t>(_pipelineShaderStages.size()),
        .pStages = _pipelineShaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .layout = _pipelineLayout,
        .renderPass = nullptr,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    // TODO: Provide formats
    vk::PipelineRenderingCreateInfoKHR renderingCreateInfo {

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
