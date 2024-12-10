#include "pipelines/lighting_pipeline.hpp"

#include "../vulkan_helper.hpp"
#include "bloom_settings.hpp"
#include "camera.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

LightingPipeline::LightingPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings, const ResourceHandle<GPUImage>& ssaoTarget)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _bloomSettings(bloomSettings)
    , _ssaoTarget(ssaoTarget)
{
    _pushConstants.albedoMIndex = _gBuffers.Attachments()[0].Index();
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
    _pushConstants.emissiveAOIndex = _gBuffers.Attachments()[2].Index();
    _pushConstants.positionIndex = _gBuffers.Attachments()[3].Index();
    _pushConstants.ssaoIndex = _ssaoTarget.Index();

    vk::PhysicalDeviceProperties properties {};
    _context->VulkanContext()->PhysicalDevice().getProperties(&properties);

    CreatePipeline();
}

void LightingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = _context->Resources()->ImageResourceManager().Access(_hdrTarget)->views[0];
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfos[0].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->views[0];
    colorAttachmentInfos[1].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[1].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[1].loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfos[1].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _gBuffers.Size().x, _gBuffers.Size().y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 3, { scene.gpuScene->GetPointLightDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 4, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

LightingPipeline::~LightingPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void LightingPipeline::CreatePipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments {};
    blendAttachments[0].blendEnable = vk::False;
    blendAttachments[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    memcpy(&blendAttachments[1], &blendAttachments[0], sizeof(vk::PipelineColorBlendAttachmentState));

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = blendAttachments.size();
    colorBlendStateCreateInfo.pAttachments = blendAttachments.data();

    std::vector<vk::Format> formats = {
        _context->Resources()->ImageResourceManager().Access(_hdrTarget)->format,
        _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->format
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/lighting.frag.spv");

    PipelineBuilder reflector { _context };
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);

    reflector.SetColorBlendState(colorBlendStateCreateInfo);
    reflector.SetColorAttachmentFormats(formats);

    reflector.BuildPipeline(_pipeline, _pipelineLayout);
}
