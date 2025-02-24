#include "passes/tonemapping_pass.hpp"

#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "settings.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

TonemappingPass::TonemappingPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, const GBuffers& gBuffers, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings)
    : _context(context)
    , _settings(settings)
    , _swapChain(_swapChain)
    , _hdrTarget(hdrTarget)
    , _bloomTarget(bloomTarget)
    , _gBuffers(gBuffers)
    , _outputTarget(outputTarget)
    , _bloomSettings(bloomSettings)
{
    CreatePipeline();

    _pushConstants.hdrTargetIndex = hdrTarget.Index();
    _pushConstants.bloomTargetIndex = bloomTarget.Index();
    _pushConstants.depthIndex = gBuffers.Depth().Index();
}

TonemappingPass::~TonemappingPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void TonemappingPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Tonemapping Pass");

    _pushConstants.exposure = _settings.exposure;
    _pushConstants.tonemappingFunction = static_cast<uint32_t>(_settings.tonemappingFunction);

    _pushConstants.enableVignette = _settings.enableVignette;
    _pushConstants.vignetteIntensity = _settings.vignetteIntensity;

    _pushConstants.enableLensDistortion = _settings.enableLensDistortion;
    _pushConstants.lensDistortionIntensity = _settings.lensDistortionIntensity;
    _pushConstants.lensDistortionCubicIntensity = _settings.lensDistortionCubicIntensity;
    _pushConstants.screenScale = _settings.screenScale;

    _pushConstants.enableToneAdjustments = _settings.enableToneAdjustments;
    _pushConstants.brightness = _settings.brightness;
    _pushConstants.contrast = _settings.contrast;
    _pushConstants.saturation = _settings.saturation;
    _pushConstants.vibrance = _settings.vibrance;
    _pushConstants.hue = _settings.hue;
    _pushConstants.minPixelSize = _settings.minPixelSize;
    _pushConstants.maxPixelSize = _settings.maxPixelSize;
    _pushConstants.pixelizationLevels = _settings.pixelizationLevels;
    _pushConstants.pixelizationDepthBias = _settings.pixelizationDepthBias;
    for (int i = 0; i < 5; i++)
    {
        _pushConstants.palette[i] = _settings.palette[i];
    }

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_outputTarget)->view,
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } },
    };

    vk::RenderingInfoKHR renderingInfo {
        .renderArea = {
            .offset = vk::Offset2D { 0, 0 },
            .extent = _swapChain.GetExtent(),
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &finalColorAttachmentInfo,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

void TonemappingPass::CreatePipeline()
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

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/tonemapping.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetColorAttachmentFormats({ _context->Resources()->ImageResourceManager().Access(_outputTarget)->format })
                      .SetDepthAttachmentFormat(vk::Format::eUndefined)
                      .BuildPipeline();

    _pipelineLayout = std::get<0>(result);
    _pipeline = std::get<1>(result);
}
