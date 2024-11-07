#include "pipelines/debug_pipeline.hpp"

#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"

#include "pipeline_builder.hpp"
#include <imgui_impl_vulkan.h>

DebugPipeline::DebugPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraResource& camera, const SwapChain& swapChain)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _swapChain(swapChain)
    , _camera(camera)
{
    _linesData.reserve(2048);
    CreateVertexBuffer();
    CreatePipeline();
}

DebugPipeline::~DebugPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    _brain.GetBufferResourceManager().Destroy(_vertexBuffer);
}

void DebugPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    UpdateVertexData();

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
    finalColorAttachmentInfo.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _brain.GetImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = _swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _camera.DescriptorSet(currentFrame) }, {});

    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_vertexBuffer);
    const std::array<vk::DeviceSize, 1> offsets = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, offsets.data());

    // Draw the lines
    commandBuffer.draw(static_cast<uint32_t>(_linesData.size()), 1, 0, 0);
    _brain.drawStats.drawCalls++;
    _brain.drawStats.debugLines = static_cast<uint32_t>(_linesData.size() / 2);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    commandBuffer.endRenderingKHR(_brain.dldi);
}

void DebugPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {};
    colorBlendAttachmentState.blendEnable = vk::False;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    std::vector<vk::Format> formats { _swapChain.GetFormat() };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eLineList;

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/debug.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/debug.frag.spv");

    PipelineBuilder pipelineBuilder { _brain };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetDepthStencilState(depthStencilStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .SetInputAssemblyState(inputAssemblyStateCreateInfo)
        .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
        .BuildPipeline(_pipeline, _pipelineLayout);
}

void DebugPipeline::CreateVertexBuffer()
{
    const vk::DeviceSize bufferSize = sizeof(glm::vec3) * 2 * 1024 * 2048;
    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(bufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(true)
        .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
        .SetName("Debug vertex buffer");

    _vertexBuffer = _brain.GetBufferResourceManager().Create(vertexBufferCreation);
}

void DebugPipeline::UpdateVertexData()
{
    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_vertexBuffer);
    memcpy(buffer->mappedPtr, _linesData.data(), _linesData.size() * sizeof(glm::vec3));
}
