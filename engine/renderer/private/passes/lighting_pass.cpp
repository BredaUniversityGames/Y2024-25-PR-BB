#include "passes/lighting_pass.hpp"

#include "bloom_settings.hpp"
#include "camera.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

LightingPass::LightingPass(const std::shared_ptr<GraphicsContext>& context, const GPUScene& scene, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings, const ResourceHandle<GPUImage>& ssaoTarget)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _bloomSettings(bloomSettings)
{
    _pushConstants.albedoMIndex = _gBuffers.Attachments()[0].Index();
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
    _pushConstants.ssaoIndex = ssaoTarget.Index();
    _pushConstants.depthIndex = _gBuffers.Depth().Index();
    _pushConstants.shadowMapSize = _context->Resources()->ImageResourceManager().Access(scene.Shadow())->width;

    vk::PhysicalDeviceProperties properties {};
    _context->VulkanContext()->PhysicalDevice().getProperties(&properties);

    CreatePipeline();
}

void LightingPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Lighting Pass");
    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = _context->Resources()->ImageResourceManager().Access(_hdrTarget)->view;
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfos[0].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->view;
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

LightingPass::~LightingPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void LightingPass::CreatePipeline()
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
