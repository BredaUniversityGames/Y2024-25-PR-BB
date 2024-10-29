#include "pipelines/lighting_pipeline.hpp"
#include "shaders/shader_loader.hpp"
#include "gpu_scene.hpp"
#include "bloom_settings.hpp"

LightingPipeline::LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, const CameraResource& camera, const BloomSettings& bloomSettings)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _camera(camera)
    , _bloomSettings(bloomSettings)
{
    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 0);

    _pushConstants.albedoMIndex = _gBuffers.Attachments()[0].index;
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].index;
    _pushConstants.emissiveAOIndex = _gBuffers.Attachments()[2].index;
    _pushConstants.positionIndex = _gBuffers.Attachments()[3].index;

    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 1);

    vk::PhysicalDeviceProperties properties {};
    _brain.physicalDevice.getProperties(&properties);

    CreatePipeline();
}

void LightingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = _brain.GetImageResourceManager().Access(_hdrTarget)->views[0];
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachmentInfos[0].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _brain.GetImageResourceManager().Access(_brightnessTarget)->views[0];
    colorAttachmentInfos[1].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[1].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[1].loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachmentInfos[1].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _gBuffers.Size().x, _gBuffers.Size().y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants), &_pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_brain.bindlessSet, 0, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _camera.DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { scene.gpuScene.GetSceneDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 3, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);
    _brain.drawStats.indexCount += 3;
    _brain.drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_brain.dldi);
}

LightingPipeline::~LightingPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}

void LightingPipeline::CreatePipeline()
{
    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments {};
    blendAttachments[0].blendEnable = vk::False;
    blendAttachments[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    memcpy(&blendAttachments[1], &blendAttachments[0], sizeof(vk::PipelineColorBlendAttachmentState));

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = blendAttachments.size();
    colorBlendStateCreateInfo.pAttachments = blendAttachments.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = false;
    depthStencilStateCreateInfo.depthWriteEnable = false;

    std::vector<vk::Format> formats = {
        _brain.GetImageResourceManager().Access(_hdrTarget)->format,
        _brain.GetImageResourceManager().Access(_brightnessTarget)->format
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/lighting.frag.spv");

    ShaderReflector reflector { _brain };
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);

    reflector.SetColorBlendState(colorBlendStateCreateInfo);
    reflector.SetDepthStencilState(depthStencilStateCreateInfo);
    reflector.SetColorAttachmentFormats(formats);
    reflector.SetDepthAttachmentFormat(_gBuffers.DepthFormat());
    reflector.SetDynamicState(dynamicStateCreateInfo);

    reflector.BuildPipeline(_pipeline, _pipelineLayout);
}
