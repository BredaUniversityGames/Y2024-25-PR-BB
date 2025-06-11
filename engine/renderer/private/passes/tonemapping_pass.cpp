#include "passes/tonemapping_pass.hpp"

#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "settings.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

TonemappingPass::TonemappingPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const GBuffers& gBuffers, const BloomSettings& bloomSettings)
    : _context(context)
    , _settings(settings)
    , _swapChain(_swapChain)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _bloomTarget(bloomTarget)
    , _outputTarget(outputTarget)
    , _bloomSettings(bloomSettings)
{
    CreatePaletteBuffer();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipeline();

    _pushConstants.hdrTargetIndex = hdrTarget.Index();
    _pushConstants.bloomTargetIndex = bloomTarget.Index();
    _pushConstants.depthIndex = gBuffers.Depth().Index();
    _pushConstants.screenWidth = _gBuffers.Size().x;
    _pushConstants.screenHeight = _gBuffers.Size().y;
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
}

TonemappingPass::~TonemappingPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_paletteDescriptorSetLayout);
    _context->Resources()->BufferResourceManager().Destroy(_colorPaletteBuffer);
}

void TonemappingPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Tonemapping Pass");

    timePassed += scene.deltaTime / 1000.0f;
    _pushConstants.exposure = _settings.exposure;
    _pushConstants.tonemappingFunction = static_cast<uint32_t>(_settings.tonemappingFunction);
    _pushConstants.rayOrigin.a -= (0.2 * (scene.deltaTime / 1000.0f));

    _pushConstants.enableFlags = 0;

    if (_settings.enableVignette)
    {
        _pushConstants.enableFlags |= eEnableVignette;
    }

    if (_settings.enableLensDistortion)
    {
        _pushConstants.enableFlags |= eEnableLensDistortion;
    }

    if (_settings.enableToneAdjustments)
    {
        _pushConstants.enableFlags |= eEnableToneAdjustments;
    }

    if (_settings.enablePixelization)
    {
        _pushConstants.enableFlags |= eEnablePixelization;
    }

    if (_settings.enablePalette)
    {
        _pushConstants.enableFlags |= eEnablePalette;
    }

    _pushConstants.vignetteIntensity = _settings.vignetteIntensity;
    _pushConstants.lensDistortionIntensity = _settings.lensDistortionIntensity;
    _pushConstants.lensDistortionCubicIntensity = _settings.lensDistortionCubicIntensity;
    _pushConstants.screenScale = _settings.screenScale;

    _pushConstants.brightness = _settings.brightness;
    _pushConstants.contrast = _settings.contrast;
    _pushConstants.saturation = _settings.saturation;
    _pushConstants.vibrance = _settings.vibrance;
    _pushConstants.hue = _settings.hue;

    float scaleFactor = _gBuffers.Size().y / 1080.0f;
    _pushConstants.minPixelSize = _settings.minPixelSize * scaleFactor;
    _pushConstants.maxPixelSize = _settings.maxPixelSize * scaleFactor;
    _pushConstants.pixelizationLevels = _settings.pixelizationLevels;
    _pushConstants.pixelizationDepthBias = _settings.pixelizationDepthBias;
    _pushConstants.ditherAmount = _settings.ditherAmount;
    _pushConstants.paletteAmount = _settings.paletteAmount;
    _pushConstants.time = timePassed;

    _pushConstants.skyColor = _settings.skyColor;
    _pushConstants.sunColor = _settings.sunColor;
    _pushConstants.cloudsColor = _settings.cloudsColor;
    _pushConstants.voidColor = _settings.voidColor;
    _pushConstants.cloudsSpeed = _settings.cloudsSpeed;
    _pushConstants.waterColor = _settings.waterColor;

    _pushConstants.paletteSize = static_cast<int>(_settings.palette.size());
    UpdatePaletteBuffer(_settings.palette);

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
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 3, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 4, { _paletteDescriptorSet }, {});

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
void TonemappingPass::CreatePaletteBuffer()
{
    std::vector<glm::vec4> colors(_maxColorsInPalette);
    vk::DeviceSize bufferSize = sizeof(glm::vec4) * _maxColorsInPalette;

    BufferCreation creation;
    creation.SetName("Palette Buffer")
        .SetSize(bufferSize)
        .SetIsMappable(true)
        .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
        .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

    _colorPaletteBuffer = _context->Resources()->BufferResourceManager().Create(creation);

    // Immediately update the buffer with the initial palette data.
    UpdatePaletteBuffer(colors);
}
void TonemappingPass::CreateDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
            .pImmutableSamplers = nullptr }
    };

    _paletteDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, { "ColorPaletteUBO" });
}

void TonemappingPass::UpdatePaletteBuffer(const std::vector<glm::vec4>& paletteColors)
{
    // Get a pointer to the mapped buffer memory.
    void* data = _context->Resources()->BufferResourceManager().Access(_colorPaletteBuffer)->mappedPtr;
    memcpy(data, paletteColors.data(), paletteColors.size() * sizeof(glm::vec4));
}

void TonemappingPass::CreateDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_paletteDescriptorSetLayout
    };

    if (_context->VulkanContext()->Device().allocateDescriptorSets(&allocInfo, &_paletteDescriptorSet) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    vk::DescriptorBufferInfo colorPaletteBufferInfo {
        .buffer = _context->Resources()->BufferResourceManager().Access(_colorPaletteBuffer)->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
        vk::WriteDescriptorSet {
            .dstSet = _paletteDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &colorPaletteBufferInfo }
    };

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
