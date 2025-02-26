#include "passes/bloom_upsample_pass.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "pipeline_builder.hpp"
#include "gpu_scene.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_helper.hpp"

BloomUpsamplePass::BloomUpsamplePass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> bloomImage)
    : _context(context), _bloomImage(bloomImage)
{
    CreatPipeline();
}

BloomUpsamplePass::~BloomUpsamplePass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void BloomUpsamplePass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Bloom Upsample Pass");

    const GPUImage* image = _context->Resources()->ImageResourceManager().Access(_bloomImage);

    glm::vec2 resolution = glm::vec2(image->width, image->height);
    for (uint32_t mip = 0; mip < image->mips - 1; ++mip)
    {
        resolution *= 0.5f;
    }

    vk::ImageMemoryBarrier2 barrier {};
    util::InitializeImageMemoryBarrier(barrier, image->image, image->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral, 1, 0, image->mips);
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite;

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier);

    commandBuffer.pipelineBarrier2(dependencyInfo);

    for (uint32_t mip = image->mips - 1; mip > 0; --mip)
    {
        uint32_t nextMip = mip - 1;

        vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
            .imageView = image->layerViews[0].mipViews[nextMip],
            .imageLayout = vk::ImageLayout::eGeneral,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };

        vk::Rect2D renderArea {
            .offset = { 0, 0 },
            .extent = { static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y) },
        };

        vk::RenderingInfoKHR renderingInfo {
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &finalColorAttachmentInfo,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr,
        };

        vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, resolution.x, resolution.y, 0.0f, 1.0f };
        commandBuffer.setViewport(0, 1, &viewport);
        commandBuffer.setScissor(0, { renderingInfo.renderArea });

        commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

        struct PushConstants
        {
            uint32_t srcImageIndex;
            uint32_t srcImageMip;
            glm::vec2 srcImageResolution;
            float filterRadius;
        } pushConstants {};
        pushConstants.srcImageIndex = _bloomImage.Index();
        pushConstants.srcImageMip = mip;
        pushConstants.srcImageResolution = resolution;
        pushConstants.filterRadius = 1.0f; // TODO: Make accessible

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
        commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pushConstants);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});

        commandBuffer.draw(3, 1, 0, 0);

        _context->GetDrawStats().Draw(3);

        commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());

        // Prepare for next pass
        resolution *= 2.0f;

        vk::ImageMemoryBarrier2 mipBarrier {};
        mipBarrier.oldLayout = vk::ImageLayout::eGeneral;
        mipBarrier.newLayout = vk::ImageLayout::eGeneral;
        mipBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        mipBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        mipBarrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        mipBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        mipBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
        mipBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        mipBarrier.image = image->image;
        mipBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        mipBarrier.subresourceRange.baseMipLevel = nextMip; // Sync only next mip for reading
        mipBarrier.subresourceRange.levelCount = 1;
        mipBarrier.subresourceRange.baseArrayLayer = 0;
        mipBarrier.subresourceRange.layerCount = 1;

        vk::DependencyInfo mipDependencyInfo {};
        mipDependencyInfo.setImageMemoryBarrierCount(1)
            .setPImageMemoryBarriers(&mipBarrier);

        commandBuffer.pipelineBarrier2(mipDependencyInfo);
    }

    // TODO: Maybe not needed
    // Make sure frame graph can transition properly
    util::TransitionImageLayout(commandBuffer, image->image, image->format, vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal , 1, 0, image->mips);
}

void BloomUpsamplePass::CreatPipeline()
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

    std::vector<vk::Format> formats { _context->Resources()->ImageResourceManager().Access(_bloomImage)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/bloom_upsampling.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .BuildPipeline();

    _pipelineLayout = std::get<0>(result);
    _pipeline = std::get<1>(result);
}
