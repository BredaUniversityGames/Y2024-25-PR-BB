#include "pipeline_builder.hpp"

#include "graphics_context.hpp"
#include "vulkan_helper.hpp"

#include "graphics_context.hpp"

std::unordered_map<size_t, vk::DescriptorSetLayout> PipelineBuilder::_cacheDescriptorSetLayouts {};

PipelineBuilder::PipelineBuilder(const std::shared_ptr<GraphicsContext>& context)
    : _context(context)
{
}

PipelineBuilder::~PipelineBuilder()
{
    for (auto& shaderStage : _shaderStages)
    {
        spvReflectDestroyShaderModule(&shaderStage.reflectModule);
        _context->VulkanContext()->Device().destroy(shaderStage.shaderModule);
    }
}

void PipelineBuilder::AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes, std::string_view entryPoint)
{
    // TODO: Handle invalid shader stages; i.e. multiple bits
    SpvReflectShaderModule reflectModule;
    util::VK_ASSERT(spvReflectCreateShaderModule(spirvBytes.size(), spirvBytes.data(), &reflectModule),
        "Failed reflecting on shader module!");

    vk::ShaderModule shaderModule = CreateShaderModule(spirvBytes);

    _shaderStages.emplace_back(ShaderStage {
        .stage = stage,
        .entryPoint = entryPoint,
        .spirvBytes = spirvBytes,
        .reflectModule = reflectModule,
        .shaderModule = shaderModule,
    });
}

std::tuple<vk::PipelineLayout, vk::Pipeline> PipelineBuilder::BuildPipeline()
{
    ReflectShaders();
    _pipelineLayout = CreatePipelineLayout();
    _pipeline = CreatePipeline();

    return { _pipelineLayout, _pipeline };
}

vk::DescriptorSetLayout PipelineBuilder::CacheDescriptorSetLayout(const VulkanContext& context, const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names)
{
    size_t hash = HashBindings(bindings, names);

    if (_cacheDescriptorSetLayouts.find(hash) == _cacheDescriptorSetLayouts.end())
    {
        vk::DescriptorSetLayoutCreateInfo layoutInfo {
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };

        vk::DescriptorSetLayout layout { context.Device().createDescriptorSetLayout(layoutInfo, nullptr) };

        _cacheDescriptorSetLayouts[hash] = layout;
    }

    return _cacheDescriptorSetLayouts[hash];
}

void PipelineBuilder::ReflectShaders()
{
    for (const auto& shaderStage : _shaderStages)
    {
        ReflectShader(shaderStage);

        ReflectPushConstants(shaderStage);
        ReflectDescriptorLayouts(shaderStage);

        vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
            .stage = shaderStage.stage,
            .module = shaderStage.shaderModule,
            .pName = shaderStage.entryPoint.data(),
        };
        _pipelineShaderStages.emplace_back(pipelineShaderStageCreateInfo);
    }
}

void PipelineBuilder::ReflectPushConstants(const PipelineBuilder::ShaderStage& shaderStage)
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

void PipelineBuilder::ReflectDescriptorLayouts(const PipelineBuilder::ShaderStage& shaderStage)
{
    uint32_t setCount { 0 };
    spvReflectEnumerateDescriptorSets(&shaderStage.reflectModule, &setCount, nullptr);
    std::vector<SpvReflectDescriptorSet*> sets { setCount };
    spvReflectEnumerateDescriptorSets(&shaderStage.reflectModule, &setCount, sets.data());

    for (const auto& set : sets)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<std::string_view> names;

        for (size_t i = 0; i < set->binding_count; ++i)
        {
            const SpvReflectDescriptorBinding* reflectBinding = set->bindings[i];
            vk::DescriptorSetLayoutBinding binding {
                .binding = reflectBinding->binding,
                .descriptorType = static_cast<vk::DescriptorType>(reflectBinding->descriptor_type),
                .descriptorCount = reflectBinding->count,
                .stageFlags = vk::ShaderStageFlagBits::eAll,
                .pImmutableSamplers = nullptr,
            };

            bindings.emplace_back(binding);
            if (reflectBinding->name != nullptr && reflectBinding->name[0] != '\0')
            {
                names.emplace_back(reflectBinding->name);
            }
            else if (reflectBinding->type_description->type_name != nullptr && reflectBinding->type_description->type_name[0] != '\0')
            {
                std::string_view name { reflectBinding->type_description->type_name };
                const char* underscorePos = std::strchr(reflectBinding->type_description->type_name, '_');
                if (underscorePos != nullptr)
                {
                    name = std::string_view { reflectBinding->type_description->type_name, static_cast<size_t>(underscorePos - reflectBinding->type_description->type_name) }; // Length up to the underscore
                }
                names.emplace_back(name);
            }
            else
            {
                bblog::warn("Failed reflecting name of descriptor set binding!");
                names.emplace_back("\0");
            }
        }

        size_t hash = HashBindings(bindings, names);

        if (_descriptorSetLayouts.size() <= set->set)
        {
            _descriptorSetLayouts.resize(set->set + 1);
        }

        if (_cacheDescriptorSetLayouts.find(hash) != _cacheDescriptorSetLayouts.end())
        {
            _descriptorSetLayouts[set->set] = _cacheDescriptorSetLayouts[hash];
        }
        else
        {
            vk::DescriptorSetLayoutCreateInfo layoutInfo {
                .bindingCount = static_cast<uint32_t>(bindings.size()),
                .pBindings = bindings.data(),
            };

            vk::DescriptorSetLayout layout { _context->VulkanContext()->Device().createDescriptorSetLayout(layoutInfo, nullptr) };

            _descriptorSetLayouts[set->set] = layout;
            _cacheDescriptorSetLayouts[hash] = _descriptorSetLayouts[set->set];
        }
    }
}

vk::PipelineLayout PipelineBuilder::CreatePipelineLayout()
{
    vk::PipelineLayoutCreateInfo createInfo {
        .setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size()),
        .pSetLayouts = _descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size()),
        .pPushConstantRanges = _pushConstantRanges.data(),
    };

    return _context->VulkanContext()->Device().createPipelineLayout(createInfo, nullptr);
}

vk::ShaderModule PipelineBuilder::CreateShaderModule(const std::vector<std::byte>& spirvBytes)
{
    vk::ShaderModuleCreateInfo createInfo {
        .codeSize = spirvBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(spirvBytes.data()),
    };

    return _context->VulkanContext()->Device().createShaderModule(createInfo, nullptr);
}

size_t PipelineBuilder::HashBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names)
{
    size_t seed = bindings.size();
    for (const auto& binding : bindings)
    {
        seed ^= std::hash<uint32_t> {}(binding.binding) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint32_t> {}(static_cast<uint32_t>(binding.descriptorType)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        // seed ^= std::hash<uint32_t> {}(binding.descriptorCount) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        // seed ^= std::hash<uint32_t>{}(static_cast<uint32_t>(binding.stageFlags)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    for (const auto& name : names)
    {
        seed ^= std::hash<std::string_view> {}(name) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const std::shared_ptr<GraphicsContext>& context)
    : PipelineBuilder(context)
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

    _depthStencilStateCreateInfo = {
        .depthTestEnable = false,
        .depthWriteEnable = false,
    };

    static constexpr std::array<vk::DynamicState, 2> DYNAMIC_STATES = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    _dynamicStateCreateInfo = {
        .dynamicStateCount = DYNAMIC_STATES.size(),
        .pDynamicStates = DYNAMIC_STATES.data(),
    };
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() = default;

std::tuple<vk::PipelineLayout, vk::Pipeline> GraphicsPipelineBuilder::BuildPipeline()
{
    return PipelineBuilder::BuildPipeline();
}

void GraphicsPipelineBuilder::ReflectShader(const ShaderStage& shaderStage)
{
    if (shaderStage.stage & vk::ShaderStageFlagBits::eVertex)
    {
        ReflectVertexInput(shaderStage);
    }
}

void GraphicsPipelineBuilder::ReflectVertexInput(const ShaderStage& shaderStage)
{
    uint32_t inputCount { 0 };

    spvReflectEnumerateInputVariables(&shaderStage.reflectModule, &inputCount, nullptr);
    std::vector<SpvReflectInterfaceVariable*> inputVariables { inputCount };
    spvReflectEnumerateInputVariables(&shaderStage.reflectModule, &inputCount, inputVariables.data());

    // Makes sure the input variables are sorted by their location.
    std::sort(inputVariables.begin(), inputVariables.end(), [](const auto a, const auto b)
        { return a->location < b->location; });

    uint32_t binding { 0 };
    vk::VertexInputBindingDescription bindingDescription {
        .binding = binding,
        .stride = 0,
        .inputRate = vk::VertexInputRate::eVertex
    };

    for (const auto* var : inputVariables)
    {
        if (var->location == std::numeric_limits<uint32_t>::max())
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

    if (!_attributeDescriptions.empty())
    {
        _bindingDescriptions.emplace_back(bindingDescription);
    }
}

vk::Pipeline GraphicsPipelineBuilder::CreatePipeline()
{
    if (!_inputAssemblyStateCreateInfo.has_value() || !_viewportStateCreateInfo.has_value() || !_rasterizationStateCreateInfo.has_value() || !_multisampleStateCreateInfo.has_value() || !_depthStencilStateCreateInfo.has_value() || !_colorBlendStateCreateInfo.has_value() || !_dynamicStateCreateInfo.has_value())
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
        .pDynamicState = &_dynamicStateCreateInfo.value(),
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

    auto [result, vkPipeline] = _context->VulkanContext()->Device().createGraphicsPipeline(nullptr, structureChain.get(), nullptr);

    util::VK_ASSERT(result, "Failed creating graphics pipeline!");

    return vkPipeline;
}

ComputePipelineBuilder::ComputePipelineBuilder(const std::shared_ptr<GraphicsContext>& context)
    : PipelineBuilder(context)
{
}

ComputePipelineBuilder::~ComputePipelineBuilder() = default;

std::tuple<vk::PipelineLayout, vk::Pipeline> ComputePipelineBuilder::BuildPipeline()
{
    return PipelineBuilder::BuildPipeline();
}

vk::Pipeline ComputePipelineBuilder::CreatePipeline()
{
    if (_pipelineShaderStages.size() > 1)
    {
        bblog::warn("Created compute pipeline that had more than one shader stages present. First one is used, others are ignored.");
    }

    vk::ComputePipelineCreateInfo computePipelineCreateInfo {
        .stage = _pipelineShaderStages.front(),
        .layout = _pipelineLayout,
    };

    auto [result, vkPipeline] = _context->VulkanContext()->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);

    util::VK_ASSERT(result, "Failed creating compute pipeline!");

    return vkPipeline;
}
