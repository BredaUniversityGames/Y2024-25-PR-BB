#include "pipelines/debug_pipeline.hpp"

#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"
#include "vulkan_context.hpp"

#include <array>
#include <imgui_impl_vulkan.h>
#include <vector>

DebugPipeline::DebugPipeline(const std::shared_ptr<VulkanContext>& context, const GBuffers& gBuffers, const CameraResource& camera, const SwapChain& swapChain)
    : _context(context)
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
    _context->Device().destroy(_pipeline);
    _context->Device().destroy(_pipelineLayout);
    _context->GetBufferResourceManager().Destroy(_vertexBuffer);
}

void DebugPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    UpdateVertexData();

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex),
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } },
    };

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
        .imageView = _context->GetImageResourceManager().Access(_gBuffers.Depth())->view,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = {
            .depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 } },
    };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {
        .renderArea = {
            .offset = vk::Offset2D { 0, 0 },
            .extent = _swapChain.GetExtent(),
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &finalColorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
        .pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr,
    };

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _camera.DescriptorSet(currentFrame) }, {});

    const Buffer* buffer = _context->GetBufferResourceManager().Access(_vertexBuffer);
    const std::array<vk::DeviceSize, 1> offsets = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, offsets.data());

    uint32_t vertexCount = static_cast<uint32_t>(_linesData.size());
    commandBuffer.draw(vertexCount, 1, 0, 0);

    _context->GetDrawStats().Draw(vertexCount);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderingKHR(_context->Dldi());
}

void DebugPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    std::vector<vk::Format> formats { _swapChain.GetFormat() };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .topology = vk::PrimitiveTopology::eLineList,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/debug.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/debug.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
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
    const vk::DeviceSize bufferSize = sizeof(glm::vec3) * 2 * 1024 * 2048; // TODO: Remove magic number.
    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(bufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(true)
        .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
        .SetName("Debug vertex buffer");

    _vertexBuffer = _context->GetBufferResourceManager().Create(vertexBufferCreation);
}

void DebugPipeline::UpdateVertexData()
{
    const Buffer* buffer = _context->GetBufferResourceManager().Access(_vertexBuffer);
    std::memcpy(buffer->mappedPtr, _linesData.data(), _linesData.size() * sizeof(glm::vec3));
}
