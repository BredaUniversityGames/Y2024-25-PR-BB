#include "pipelines/ibl_pipeline.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"

IBLPipeline::IBLPipeline(const VulkanBrain& brain, ResourceHandle<Image> environmentMap)
    : _brain(brain)
    , _environmentMap(environmentMap)
{
    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge, vk::SamplerMipmapMode::eLinear, 0).release();

    CreateIrradianceCubemap();
    CreatePrefilterCubemap();
    CreateBRDFLUT();
    CreateDescriptorSetLayout();
    CreateDescriptorSet();
    CreateIrradiancePipeline();
    CreatePrefilterPipeline();
    CreateBRDFLUTPipeline();
}

IBLPipeline::~IBLPipeline()
{
    for (const auto& mips : _prefilterMapViews)
        for (const auto& view : mips)
            _brain.device.destroy(view);

    _brain.GetImageResourceManager().Destroy(_irradianceMap);
    _brain.GetImageResourceManager().Destroy(_brdfLUT);
    _brain.GetImageResourceManager().Destroy(_prefilterMap);

    _brain.device.destroy(_sampler);

    _brain.device.destroy(_prefilterPipeline);
    _brain.device.destroy(_prefilterPipelineLayout);
    _brain.device.destroy(_irradiancePipeline);
    _brain.device.destroy(_irradiancePipelineLayout);
    _brain.device.destroy(_brdfLUTPipeline);
    _brain.device.destroy(_brdfLUTPipelineLayout);
    _brain.device.destroy(_descriptorSetLayout);
}

void IBLPipeline::RecordCommands(vk::CommandBuffer commandBuffer)
{
    const Image& irradianceMap = *_brain.GetImageResourceManager().Access(_irradianceMap);
    const Image& prefilterMap = *_brain.GetImageResourceManager().Access(_prefilterMap);

    util::BeginLabel(commandBuffer, "Irradiance pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _brain.dldi);

    util::TransitionImageLayout(commandBuffer, irradianceMap.image, irradianceMap.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 6, 0, 1);

    for (size_t i = 0; i < 6; ++i)
    {
        vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
        finalColorAttachmentInfo.imageView = irradianceMap.views[i];
        finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;
        finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;

        vk::RenderingInfoKHR renderingInfo {};
        renderingInfo.renderArea.extent = vk::Extent2D { static_cast<uint32_t>(irradianceMap.width), static_cast<uint32_t>(irradianceMap.height) };
        renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
        renderingInfo.layerCount = 1;
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

        commandBuffer.pushConstants(_irradiancePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), &i);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _irradiancePipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _irradiancePipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

        vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(irradianceMap.width), static_cast<float>(irradianceMap.height), 0.0f,
            1.0f };
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Extent2D extent = vk::Extent2D { static_cast<uint32_t>(irradianceMap.width), static_cast<uint32_t>(irradianceMap.height) };
        vk::Rect2D scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, extent };
        commandBuffer.setScissor(0, 1, &scissor);

        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRenderingKHR(_brain.dldi);
    }

    util::TransitionImageLayout(commandBuffer, irradianceMap.image, irradianceMap.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 6, 0, 1);
    util::EndLabel(commandBuffer, _brain.dldi);

    util::BeginLabel(commandBuffer, "Prefilter pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _brain.dldi);
    util::TransitionImageLayout(commandBuffer, prefilterMap.image, prefilterMap.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 6, 0, prefilterMap.mips);

    for (size_t i = 0; i < prefilterMap.mips; ++i)
    {
        for (size_t j = 0; j < 6; ++j)
        {
            vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
            finalColorAttachmentInfo.imageView = _prefilterMapViews[i][j];
            finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;
            finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
            finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;

            uint32_t size = static_cast<uint32_t>(prefilterMap.width >> i);

            vk::RenderingInfoKHR renderingInfo {};
            renderingInfo.renderArea.extent = vk::Extent2D { size, size };
            renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
            renderingInfo.layerCount = 1;
            renderingInfo.pDepthAttachment = nullptr;
            renderingInfo.pStencilAttachment = nullptr;

            commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

            PrefilterPushConstant pc { static_cast<uint32_t>(j), static_cast<float>(i) / static_cast<float>(prefilterMap.mips - 1) };

            commandBuffer.pushConstants(_prefilterPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(PrefilterPushConstant), &pc);
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _prefilterPipeline);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _prefilterPipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

            vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size), 0.0f,
                1.0f };
            commandBuffer.setViewport(0, 1, &viewport);

            vk::Extent2D extent = vk::Extent2D { static_cast<uint32_t>(size), static_cast<uint32_t>(size) };
            vk::Rect2D scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, extent };
            commandBuffer.setScissor(0, 1, &scissor);

            commandBuffer.draw(3, 1, 0, 0);

            commandBuffer.endRenderingKHR(_brain.dldi);
        }
    }

    util::TransitionImageLayout(commandBuffer, prefilterMap.image, prefilterMap.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 6, 0, prefilterMap.mips);

    util::EndLabel(commandBuffer, _brain.dldi);

    const Image* brdfLUT = _brain.GetImageResourceManager().Access(_brdfLUT);
    util::BeginLabel(commandBuffer, "BRDF Integration pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _brain.dldi);
    util::TransitionImageLayout(commandBuffer, brdfLUT->image, brdfLUT->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = _brain.GetImageResourceManager().Access(_brdfLUT)->views[0];
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimal;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { brdfLUT->width, brdfLUT->height };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _brdfLUTPipeline);

    vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(brdfLUT->width), static_cast<float>(brdfLUT->height), 0.0f,
        1.0f };
    commandBuffer.setViewport(0, 1, &viewport);

    vk::Extent2D extent = vk::Extent2D { static_cast<uint32_t>(brdfLUT->width), static_cast<uint32_t>(brdfLUT->height) };
    vk::Rect2D scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, extent };
    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.draw(3, 1, 0, 0);

    commandBuffer.endRenderingKHR(_brain.dldi);

    util::TransitionImageLayout(commandBuffer, brdfLUT->image, brdfLUT->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void IBLPipeline::CreateIrradiancePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 1> layouts = { _descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    vk::PushConstantRange pushConstantRange {};
    pushConstantRange.size = sizeof(uint32_t) * 6;
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_irradiancePipelineLayout),
        "Failed to create IBL pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/bin/irradiance.vert.spv");
    auto fragByteCode = shader::ReadFile("shaders/bin/irradiance.frag.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragByteCode, _brain.device);

    vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo {};
    vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo {};
    fragShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageCreateInfo.module = fragModule;
    fragShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = vk::False;

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
    rasterizationStateCreateInfo.depthClampEnable = vk::False;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationStateCreateInfo.depthBiasEnable = vk::False;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;

    std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

    auto& pipelineCreateInfo = structureChain.get<vk::GraphicsPipelineCreateInfo>();
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = _irradiancePipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &_brain.GetImageResourceManager().Access(_irradianceMap)->format;

    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the IBL pipeline!");
    _irradiancePipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void IBLPipeline::CreatePrefilterPipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 1> layouts = { _descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    vk::PushConstantRange pushConstantRange {};
    pushConstantRange.size = sizeof(PrefilterPushConstant) * 6 * _brain.GetImageResourceManager().Access(_prefilterMap)->mips;
    assert(pushConstantRange.size < 256);
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_prefilterPipelineLayout),
        "Failed to create IBL pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/bin/prefilter.vert.spv");
    auto fragByteCode = shader::ReadFile("shaders/bin/prefilter.frag.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragByteCode, _brain.device);

    vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo {};
    vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo {};
    fragShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageCreateInfo.module = fragModule;
    fragShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = vk::False;

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
    rasterizationStateCreateInfo.depthClampEnable = vk::False;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationStateCreateInfo.depthBiasEnable = vk::False;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;

    std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

    auto& pipelineCreateInfo = structureChain.get<vk::GraphicsPipelineCreateInfo>();
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = _prefilterPipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &_brain.GetImageResourceManager().Access(_prefilterMap)->format;

    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the IBL pipeline!");
    _prefilterPipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void IBLPipeline::CreateBRDFLUTPipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 1> layouts = { _descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_brdfLUTPipelineLayout),
        "Failed to create IBL pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/bin/brdf_integration.vert.spv");
    auto fragByteCode = shader::ReadFile("shaders/bin/brdf_integration.frag.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragByteCode, _brain.device);

    vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo {};
    vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo {};
    fragShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageCreateInfo.module = fragModule;
    fragShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = vk::False;

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
    rasterizationStateCreateInfo.depthClampEnable = vk::False;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationStateCreateInfo.depthBiasEnable = vk::False;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;

    std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

    auto& pipelineCreateInfo = structureChain.get<vk::GraphicsPipelineCreateInfo>();
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = _brdfLUTPipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &_brain.GetImageResourceManager().Access(_brdfLUT)->format;

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the IBL pipeline!");
    _brdfLUTPipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void IBLPipeline::CreateDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

    vk::DescriptorSetLayoutBinding& samplerLayoutBinding { bindings[0] };
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_descriptorSetLayout),
        "Failed creating IBL descriptor set layout!");
}

void IBLPipeline::CreateDescriptorSet()
{
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_descriptorSetLayout;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, &_descriptorSet),
        "Failed allocating descriptor sets!");

    vk::DescriptorImageInfo imageInfo {};
    imageInfo.sampler = _sampler;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = _brain.GetImageResourceManager().Access(_environmentMap)->view;

    vk::WriteDescriptorSet descriptorWrite {};
    descriptorWrite.dstSet = _descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    _brain.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void IBLPipeline::CreateIrradianceCubemap()
{
    ImageCreation creation {};
    creation
        .SetName("Irradiance cubemap")
        .SetType(ImageType::eCubeMap)
        .SetSize(32, 32)
        .SetFormat(vk::Format::eR16G16B16A16Sfloat)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _irradianceMap = _brain.GetImageResourceManager().Create(creation);
}

void IBLPipeline::CreatePrefilterCubemap()
{
    ImageCreation creation {};
    creation
        .SetName("Prefilter cubemap")
        .SetType(ImageType::eCubeMap)
        .SetSize(128, 128)
        .SetFormat(vk::Format::eR16G16B16A16Sfloat)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled)
        .SetSampler(_sampler)
        .SetMips(fmin(floor(log2(creation.width)), 3.0));

    _prefilterMap = _brain.GetImageResourceManager().Create(creation);

    _prefilterMapViews.resize(creation.mips);
    for (size_t i = 0; i < _prefilterMapViews.size(); ++i)
    {
        for (size_t j = 0; j < 6; ++j)
        {
            vk::ImageViewCreateInfo imageViewCreateInfo {};
            imageViewCreateInfo.image = _brain.GetImageResourceManager().Access(_prefilterMap)->image;
            imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
            imageViewCreateInfo.format = creation.format;
            imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageViewCreateInfo.subresourceRange.baseMipLevel = i;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = j;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            util::VK_ASSERT(_brain.device.createImageView(&imageViewCreateInfo, nullptr, &_prefilterMapViews[i][j]), "Failed creating irradiance map image view!");
        }
    }
}

void IBLPipeline::CreateBRDFLUT()
{
    ImageCreation creation {};
    creation
        .SetName("BRDF LUT")
        .SetSize(512, 512)
        .SetFormat(vk::Format::eR16G16Sfloat)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
    _brdfLUT = _brain.GetImageResourceManager().Create(creation);
}
