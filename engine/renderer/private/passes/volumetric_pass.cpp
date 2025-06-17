#include "passes/volumetric_pass.hpp"

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

#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

VolumetricPass::VolumetricPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const GBuffers& gBuffers, const BloomSettings& bloomSettings, ECSModule& ecs)
    : _context(context)
    , _settings(settings)
    , _swapChain(_swapChain)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _bloomTarget(bloomTarget)
    , _outputTarget(outputTarget)
    , _bloomSettings(bloomSettings)
    , _ecs(ecs)
{
    CreateFogTrailsBuffer();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipeline();

    _pushConstants.hdrTargetIndex = hdrTarget.Index();
    _pushConstants.bloomTargetIndex = bloomTarget.Index();
    _pushConstants.depthIndex = gBuffers.Depth().Index();
    _pushConstants.screenWidth = _gBuffers.Size().x / 4.0;
    _pushConstants.screenHeight = _gBuffers.Size().y / 4.0;
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();

    for (size_t i = 0; i < gunShots.size(); ++i)
    {
        gunShots[i].origin = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        gunShots[i].direction = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    for (size_t i = 0; i < playerTrailPositions.size(); ++i)
    {
        playerTrailPositions[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

VolumetricPass::~VolumetricPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_fogTrailsDescriptorSetLayout);
    _context->Resources()->BufferResourceManager().Destroy(_fogTrailsBuffer);
}

void VolumetricPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Tonemapping Pass");

    timePassed += scene.deltaTime / 1000.0f;

    for (uint32_t i = 0; i < gunShots.size(); ++i)
    {
        gunShots[i].origin.a -= (0.2 * (scene.deltaTime / 1000.0f));
    }

    // leave a delayed trail positions for the player
    for (uint32_t i = 0; i < playerTrailPositions.size(); ++i)
    {
        playerTrailPositions[i].a -= (0.45 * (scene.deltaTime / 1000.0f));

        _playerPosCounterMs = 0;
        // Update the player position trail
        auto cameraView = _ecs.GetRegistry().view<CameraComponent, TransformComponent>();
        for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
        {

            auto position = TransformHelpers::GetWorldPosition(_ecs.GetRegistry(), entity);
            AddPlayerPos(position);
            break; // we only need the player
        }

        _playerPosCounterMs += scene.deltaTime;
    }

    _pushConstants.time = timePassed;
    UpdateFogTrailsBuffer();

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
            .extent = vk::Extent2D { _gBuffers.Size().x / 6, _gBuffers.Size().y / 6 } },
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
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 4, { _fogTrailsDescriptorSet }, {});

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

void VolumetricPass::CreatePipeline()
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
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/volumetric.frag.spv");

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
void VolumetricPass::UpdateFogTrailsBuffer()
{
    void* data = _context->Resources()->BufferResourceManager().Access(_fogTrailsBuffer)->mappedPtr;
    memcpy(data, gunShots.data(), gunShots.size() * sizeof(glm::vec4) * 2);
    memcpy(reinterpret_cast<std::byte*>(data) + gunShots.size() * sizeof(GunShot), playerTrailPositions.data(), playerTrailPositions.size() * sizeof(glm::vec4));
}
void VolumetricPass::CreateFogTrailsBuffer()
{
    const vk::DeviceSize bufferSize = (gunShots.size() * sizeof(glm::vec4) * 2) + (playerTrailPositions.size() * sizeof(glm::vec4));

    BufferCreation creation;
    creation.SetName("Fog Trails Buffer")
        .SetSize(bufferSize)
        .SetIsMappable(true)
        .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
        .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

    _fogTrailsBuffer = _context->Resources()->BufferResourceManager().Create(creation);

    // Immediately update the buffer with the initial palette data.
    UpdateFogTrailsBuffer();
}
void VolumetricPass::CreateDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
            .pImmutableSamplers = nullptr }
    };

    _fogTrailsDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, { "FogTrailsUBO" });
}
void VolumetricPass::CreateDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_fogTrailsDescriptorSetLayout
    };

    if (_context->VulkanContext()->Device().allocateDescriptorSets(&allocInfo, &_fogTrailsDescriptorSet) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    vk::DescriptorBufferInfo colorPaletteBufferInfo {
        .buffer = _context->Resources()->BufferResourceManager().Access(_fogTrailsBuffer)->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
        vk::WriteDescriptorSet {
            .dstSet = _fogTrailsDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &colorPaletteBufferInfo }
    };

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
