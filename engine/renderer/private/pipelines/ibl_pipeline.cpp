#include "pipelines/ibl_pipeline.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"
#include "vulkan_helper.hpp"

IBLPipeline::IBLPipeline(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Image> environmentMap)
    : _context(context)
    , _environmentMap(environmentMap)
{
    _sampler = util::CreateSampler(_context, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge, vk::SamplerMipmapMode::eLinear, 0).release();

    CreateIrradianceCubemap();
    CreatePrefilterCubemap();
    CreateBRDFLUT();
    CreateIrradiancePipeline();
    CreatePrefilterPipeline();
    CreateBRDFLUTPipeline();
}

IBLPipeline::~IBLPipeline()
{
    for (const auto& mips : _prefilterMapViews)
        for (const auto& view : mips)
            _context->Device().destroy(view);

    _context->GetImageResourceManager().Destroy(_irradianceMap);
    _context->GetImageResourceManager().Destroy(_brdfLUT);
    _context->GetImageResourceManager().Destroy(_prefilterMap);

    _context->Device().destroy(_sampler);

    _context->Device().destroy(_prefilterPipeline);
    _context->Device().destroy(_prefilterPipelineLayout);
    _context->Device().destroy(_irradiancePipeline);
    _context->Device().destroy(_irradiancePipelineLayout);
    _context->Device().destroy(_brdfLUTPipeline);
    _context->Device().destroy(_brdfLUTPipelineLayout);
}

void IBLPipeline::RecordCommands(vk::CommandBuffer commandBuffer)
{
    const Image& irradianceMap = *_context->GetImageResourceManager().Access(_irradianceMap);
    const Image& prefilterMap = *_context->GetImageResourceManager().Access(_prefilterMap);

    util::BeginLabel(commandBuffer, "Irradiance pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _context->Dldi());

    util::TransitionImageLayout(commandBuffer, irradianceMap.image, irradianceMap.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 6, 0, 1);

    for (size_t i = 0; i < 6; ++i)
    {
        vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
            .imageView = irradianceMap.views[i],
            .imageLayout = vk::ImageLayout::eAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };

        vk::RenderingInfoKHR renderingInfo {
            .renderArea = {
                .offset = vk::Offset2D { 0, 0 },
                .extent = vk::Extent2D { static_cast<uint32_t>(irradianceMap.width), static_cast<uint32_t>(irradianceMap.height) },
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &finalColorAttachmentInfo,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr,
        };

        commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

        IrradiancePushConstant pc {
            .index = static_cast<uint32_t>(i),
            .hdriIndex = _environmentMap.index,
        };

        commandBuffer.pushConstants<IrradiancePushConstant>(_irradiancePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, { pc });
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _irradiancePipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _irradiancePipelineLayout, 0, { _context->BindlessSet() }, {});

        vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(irradianceMap.width), static_cast<float>(irradianceMap.height), 0.0f,
            1.0f };
        commandBuffer.setViewport(0, 1, &viewport);
        commandBuffer.setScissor(0, { renderingInfo.renderArea });

        commandBuffer.draw(3, 1, 0, 0);

        _context->GetDrawStats().Draw(3);

        commandBuffer.endRenderingKHR(_context->Dldi());
    }

    util::TransitionImageLayout(commandBuffer, irradianceMap.image, irradianceMap.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 6, 0, 1);
    util::EndLabel(commandBuffer, _context->Dldi());

    util::BeginLabel(commandBuffer, "Prefilter pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _context->Dldi());
    util::TransitionImageLayout(commandBuffer, prefilterMap.image, prefilterMap.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 6, 0, prefilterMap.mips);

    for (size_t i = 0; i < prefilterMap.mips; ++i)
    {
        for (size_t j = 0; j < 6; ++j)
        {
            vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
                .imageView = _prefilterMapViews[i][j],
                .imageLayout = vk::ImageLayout::eAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eDontCare,
                .storeOp = vk::AttachmentStoreOp::eStore,
            };
            uint32_t size = static_cast<uint32_t>(prefilterMap.width >> i);

            vk::RenderingInfoKHR renderingInfo {
                .renderArea = {
                    .offset = vk::Offset2D { 0, 0 },
                    .extent = vk::Extent2D { size, size },
                },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &finalColorAttachmentInfo,
                .pDepthAttachment = nullptr,
                .pStencilAttachment = nullptr,
            };

            commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

            PrefilterPushConstant pc {
                .faceIndex = static_cast<uint32_t>(j),
                .roughness = static_cast<float>(i) / static_cast<float>(prefilterMap.mips - 1),
                .hdriIndex = _environmentMap.index,
            };

            commandBuffer.pushConstants<PrefilterPushConstant>(_prefilterPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, { pc });
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _prefilterPipeline);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _prefilterPipelineLayout, 0, { _context->BindlessSet() }, {});

            vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size), 0.0f,
                1.0f };
            commandBuffer.setViewport(0, 1, &viewport);
            commandBuffer.setScissor(0, { renderingInfo.renderArea });

            commandBuffer.draw(3, 1, 0, 0);

            _context->GetDrawStats().Draw(3);

            commandBuffer.endRenderingKHR(_context->Dldi());
        }
    }

    util::TransitionImageLayout(commandBuffer, prefilterMap.image, prefilterMap.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 6, 0, prefilterMap.mips);

    util::EndLabel(commandBuffer, _context->Dldi());

    const Image* brdfLUT = _context->GetImageResourceManager().Access(_brdfLUT);
    util::BeginLabel(commandBuffer, "BRDF Integration pass", glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f, _context->Dldi());
    util::TransitionImageLayout(commandBuffer, brdfLUT->image, brdfLUT->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _context->GetImageResourceManager().Access(_brdfLUT)->views[0],
        .imageLayout = vk::ImageLayout::eAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfoKHR renderingInfo {
        .renderArea = {
            .offset = vk::Offset2D { 0, 0 },
            .extent = vk::Extent2D { brdfLUT->width, brdfLUT->height },
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &finalColorAttachmentInfo,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _brdfLUTPipeline);

    vk::Viewport viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(brdfLUT->width), static_cast<float>(brdfLUT->height), 0.0f,
        1.0f };

    commandBuffer.setViewport(0, { viewport });
    commandBuffer.setScissor(0, { renderingInfo.renderArea });

    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->Dldi());

    util::TransitionImageLayout(commandBuffer, brdfLUT->image, brdfLUT->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    util::EndLabel(commandBuffer, _context->Dldi());
}

void IBLPipeline::CreateIrradiancePipeline()
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

    std::vector<vk::Format> formats { _context->GetImageResourceManager().Access(_irradianceMap)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/irradiance.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .BuildPipeline(_irradiancePipeline, _irradiancePipelineLayout);
}

void IBLPipeline::CreatePrefilterPipeline()
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

    std::vector<vk::Format> formats { _context->GetImageResourceManager().Access(_prefilterMap)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/prefilter.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .BuildPipeline(_prefilterPipeline, _prefilterPipelineLayout);
}

void IBLPipeline::CreateBRDFLUTPipeline()
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

    std::vector<vk::Format> formats { _context->GetImageResourceManager().Access(_brdfLUT)->format };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/brdf_integration.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .BuildPipeline(_brdfLUTPipeline, _brdfLUTPipelineLayout);
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

    _irradianceMap = _context->GetImageResourceManager().Create(creation);
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

    _prefilterMap = _context->GetImageResourceManager().Create(creation);

    _prefilterMapViews.resize(creation.mips);
    for (size_t i = 0; i < _prefilterMapViews.size(); ++i)
    {
        for (size_t j = 0; j < 6; ++j)
        {
            vk::ImageViewCreateInfo imageViewCreateInfo {};
            imageViewCreateInfo.image = _context->GetImageResourceManager().Access(_prefilterMap)->image;
            imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
            imageViewCreateInfo.format = creation.format;
            imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageViewCreateInfo.subresourceRange.baseMipLevel = i;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = j;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            util::VK_ASSERT(_context->Device().createImageView(&imageViewCreateInfo, nullptr, &_prefilterMapViews[i][j]), "Failed creating irradiance map image view!");
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
    _brdfLUT = _context->GetImageResourceManager().Create(creation);
}
