#include "passes/gaussian_blur_pass.hpp"

#include "../vulkan_helper.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

#include <string>
#include <vector>

GaussianBlurPass::GaussianBlurPass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> source, ResourceHandle<GPUImage> target)
    : _context(context)
    , _source(source)
{
    // The result target will be the vertical target, as the vertical pass is the last one
    _targets[1] = target;
    CreateVerticalTarget();

    SamplerCreation createInfo {
        .name = "Gaussian blur sampler",
        .borderColor = vk::BorderColor::eFloatOpaqueBlack,
        .maxLod = 1.0f,
    };
    createInfo.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToBorder);
    _sampler = _context->Resources()->SamplerResourceManager().Create(createInfo);
    CreatePipeline();
    CreateDescriptorSets();
}

GaussianBlurPass::~GaussianBlurPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_descriptorSetLayout);
}

void GaussianBlurPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Gaussian Blur Pass");
    //  The verticalTargetData target is created by this pass, so we need to transition it from undefined layout
    const GPUImage* verticalTarget = _context->Resources()->ImageResourceManager().Access(_targets[0]);
    util::TransitionImageLayout(commandBuffer, verticalTarget->image, verticalTarget->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    vk::DescriptorSet descriptorSet = _sourceDescriptorSets[currentFrame];

    const uint32_t blurPasses = 5; // TODO: Get from bloom settings from ECS
    for (uint32_t i = 0; i < blurPasses * 2; ++i)
    {
        uint32_t isVerticalPass = i % 2;
        const GPUImage* target = _context->Resources()->ImageResourceManager().Access(_targets[isVerticalPass]);

        // We don't transition on first horizontal pass, since the first source are not either of the blur targets
        // We also don't need to update the descriptor set, since on the first horizontal pass we want to sample from the source
        if (i != 0)
        {
            uint32_t horizontalTargetIndex = isVerticalPass ? 0 : 1;
            descriptorSet = _targetDescriptorSets[horizontalTargetIndex][currentFrame];
            const GPUImage* source = _context->Resources()->ImageResourceManager().Access(_targets[horizontalTargetIndex]);

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

        commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

        commandBuffer.pushConstants<uint32_t>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, isVerticalPass);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { descriptorSet }, {});

        // Fullscreen triangle
        commandBuffer.draw(3, 1, 0, 0);

        _context->GetDrawStats().Draw(3);

        commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
    }
}

void GaussianBlurPass::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    std::vector<vk::Format> formats { _context->Resources()->ImageResourceManager().Access(_source)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/gaussian_blur.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .BuildPipeline();

    _pipelineLayout = std::get<0>(result);
    _pipeline = std::get<1>(result);

    _descriptorSetLayout = pipelineBuilder.GetDescriptorSetLayouts()[0];
}

void GaussianBlurPass::CreateDescriptorSetLayout()
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

    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&createInfo, nullptr, &_descriptorSetLayout),
        "Failed creating gaussian blur descriptor set layout!");
}

void GaussianBlurPass::CreateDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data(),
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, _sourceDescriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorImageInfo imageInfo {
            .sampler = _context->Resources()->SamplerResourceManager().Access(_sampler)->sampler,
            .imageView = _context->Resources()->ImageResourceManager().Access(_source)->views[0],
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};
        descriptorWrites[0].dstSet = _sourceDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    for (size_t i = 0; i < _targets.size(); ++i)
    {
        util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, _targetDescriptorSets[i].data()),
            "Failed allocating descriptor sets!");

        for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
        {
            vk::DescriptorImageInfo imageInfo {
                .sampler = _context->Resources()->SamplerResourceManager().Access(_sampler)->sampler,
                .imageView = _context->Resources()->ImageResourceManager().Access(_targets[i])->views[0],
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            };

            std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};
            descriptorWrites[0].dstSet = _targetDescriptorSets[i][frame];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;

            _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }
}

void GaussianBlurPass::CreateVerticalTarget()
{
    const GPUImage* horizontalTargetAccess = _context->Resources()->ImageResourceManager().Access(_targets[1]);
    std::string verticalTargetName = std::string(horizontalTargetAccess->name + " | vertical");

    CPUImage verticalTargetData {};
    verticalTargetData.SetName(verticalTargetName).SetSize(horizontalTargetAccess->width, horizontalTargetAccess->height).SetFormat(horizontalTargetAccess->format).SetFlags(horizontalTargetAccess->flags);

    _targets[0] = _context->Resources()->ImageResourceManager().Create(verticalTargetData);
}
