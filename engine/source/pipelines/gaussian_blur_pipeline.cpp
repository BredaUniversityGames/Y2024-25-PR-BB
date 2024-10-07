#include "pipelines/gaussian_blur_pipeline.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"

GaussianBlurPipeline::GaussianBlurPipeline(const VulkanBrain& brain, ResourceHandle<Image> source, ResourceHandle<Image> target)
    : _brain(brain)
    , _source(source)
{
    // The result target will be the vertical target, as the vertical pass is the last one
    _targets[1] = target;
    CreateVerticalTarget();

    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 1);
    CreateDescriptorSetLayout();
    CreateDescriptorSets();
    CreatePipeline();
}

GaussianBlurPipeline::~GaussianBlurPipeline()
{
    _brain.GetImageResourceManager().Destroy(_targets[0]);

    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    _brain.device.destroy(_descriptorSetLayout);
}

void GaussianBlurPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t blurPasses)
{
    util::BeginLabel(commandBuffer, "Gaussian blur pass", glm::vec3 { 255.0f, 255.0f, 153.0f } / 255.0f, _brain.dldi);

    // The horizontal target is created by this pass, so we need to transition it from undefined layout
    auto verticalTarget = _brain.GetImageResourceManager().Access(_targets[0]);
    util::TransitionImageLayout(commandBuffer, verticalTarget->image, verticalTarget->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    auto descriptorSet = &_sourceDescriptorSets[currentFrame];

    for (uint32_t i = 0; i < blurPasses * 2; ++i)
    {
        uint32_t isVerticalPass = i % 2;
        auto target = _brain.GetImageResourceManager().Access(_targets[isVerticalPass]);

        // We don't transition on first horizontal pass, since the first source are not either of the blur targets
        // We also don't need to update the descriptor set, since on the first horizontal pass we want to sample from the source
        if (i != 0)
        {
            uint32_t horizontalTargetIndex = isVerticalPass ? 0 : 1;
            descriptorSet = &_targetDescriptorSets[horizontalTargetIndex][currentFrame];
            auto source = _brain.GetImageResourceManager().Access(_targets[horizontalTargetIndex]);

            util::TransitionImageLayout(commandBuffer, source->image, source->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            // The first vertical pass, the target is already set up for color attachment
            if (i != 1)
            {
                util::TransitionImageLayout(commandBuffer, target->image, target->format, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal);
            }
        }

        vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
        finalColorAttachmentInfo.imageView = target->views[0];
        finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
        finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        finalColorAttachmentInfo.clearValue.color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f };

        vk::RenderingInfoKHR renderingInfo {};
        renderingInfo.renderArea.extent = vk::Extent2D { target->width, target->height };
        renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
        renderingInfo.layerCount = 1;
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uint32_t), &isVerticalPass);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, descriptorSet, 0, nullptr);

        // Fullscreen triangle
        commandBuffer.draw(3, 1, 0, 0);
        _brain.drawStats.indexCount += 3;
        _brain.drawStats.drawCalls++;
        commandBuffer.endRenderingKHR(_brain.dldi);
    }

    util::EndLabel(commandBuffer, _brain.dldi);
}

void GaussianBlurPipeline::CreatePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &_descriptorSetLayout;

    vk::PushConstantRange pushConstantRange {};
    pushConstantRange.size = sizeof(uint32_t);
    pushConstantRange.offset = 0;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed creating geometry pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    auto fragByteCode = shader::ReadFile("shaders/bin/gaussian_blur.frag.spv");

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

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {};
    colorBlendAttachmentState.blendEnable = vk::False;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = false;
    depthStencilStateCreateInfo.depthWriteEnable = false;

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
    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    vk::Format format = _brain.GetImageResourceManager().Access(_source)->format;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the geometry pipeline layout!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void GaussianBlurPipeline::CreateDescriptorSetLayout()
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
        "Failed creating gaussian blur descriptor set layout!");
}

void GaussianBlurPipeline::CreateDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, _sourceDescriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorImageInfo imageInfo {};
        imageInfo.sampler = *_sampler;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = _brain.GetImageResourceManager().Access(_source)->views[0];

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};
        descriptorWrites[0].dstSet = _sourceDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    for (size_t i = 0; i < _targets.size(); ++i)
    {
        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, _targetDescriptorSets[i].data()),
            "Failed allocating descriptor sets!");

        for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            vk::DescriptorImageInfo imageInfo {};
            imageInfo.sampler = *_sampler;
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = _brain.GetImageResourceManager().Access(_targets[i])->views[0];

            std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};
            descriptorWrites[0].dstSet = _targetDescriptorSets[i][frame];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;

            _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
}

void GaussianBlurPipeline::CreateVerticalTarget()
{
    auto horizontalTargetAccess = _brain.GetImageResourceManager().Access(_targets[1]);
    std::string verticalTargetName = std::string(horizontalTargetAccess->name + " | vertical");

    ImageCreation verticalTargetCreation {};
    verticalTargetCreation.SetName(verticalTargetName).SetSize(horizontalTargetAccess->width, horizontalTargetAccess->height).SetFormat(horizontalTargetAccess->format).SetFlags(horizontalTargetAccess->flags);

    _targets[0] = _brain.GetImageResourceManager().Create(verticalTargetCreation);
}
