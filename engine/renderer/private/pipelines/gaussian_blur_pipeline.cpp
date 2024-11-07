#include "pipelines/gaussian_blur_pipeline.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_helper.hpp"

GaussianBlurPipeline::GaussianBlurPipeline(const VulkanContext& brain, ResourceHandle<Image> source, ResourceHandle<Image> target)
    : _brain(brain)
    , _source(source)
{
    // The result target will be the vertical target, as the vertical pass is the last one
    _targets[1] = target;
    CreateVerticalTarget();

    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 1);
    CreatePipeline();
    CreateDescriptorSets();
}

GaussianBlurPipeline::~GaussianBlurPipeline()
{
    _brain.GetImageResourceManager().Destroy(_targets[0]);

    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    _brain.device.destroy(_descriptorSetLayout);
}

void GaussianBlurPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    // The vertical target is created by this pass, so we need to transition it from undefined layout
    auto verticalTarget = _brain.GetImageResourceManager().Access(_targets[0]);
    util::TransitionImageLayout(commandBuffer, verticalTarget->image, verticalTarget->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    auto descriptorSet = &_sourceDescriptorSets[currentFrame];

    const uint32_t blurPasses = 5; // TODO: Get from bloom settings from ECS
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

        vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
            .imageView = target->views[0],
            .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = { { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } },
        };

        vk::Rect2D renderArea {
            .offset = { 0, 0 },
            .extent = { target->width, target->height },
        };

        vk::RenderingInfoKHR renderingInfo {
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &finalColorAttachmentInfo,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr,
        };

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
}

void GaussianBlurPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {};
    colorBlendAttachmentState.blendEnable = vk::False;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    std::vector<vk::Format> formats { _brain.GetImageResourceManager().Access(_source)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/gaussian_blur.frag.spv");

    PipelineBuilder pipelineBuilder { _brain };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .BuildPipeline(_pipeline, _pipelineLayout);

    _descriptorSetLayout = pipelineBuilder.GetDescriptorSetLayouts()[0];
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
