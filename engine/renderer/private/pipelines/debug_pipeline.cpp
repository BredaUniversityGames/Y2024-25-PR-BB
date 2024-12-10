#include "pipelines/debug_pipeline.hpp"

#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"
#include "vulkan_context.hpp"
#include <vulkan_helper.hpp>

#include <array>
#include <imgui_impl_vulkan.h>
#include <vector>

DebugPipeline::DebugPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, ResourceHandle<GPUImage> uiTarget, const SwapChain& swapChain)
    : _context(context)
    , _gBuffers(gBuffers)
    , _swapChain(swapChain)
    , _uiTarget(uiTarget)
{
    _linesData.reserve(2048);
    CreateVertexBuffer();
    CreatePipeline();
}

DebugPipeline::~DebugPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void DebugPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    UpdateVertexData();

    const GPUImage* uiTarget = _context->Resources()->ImageResourceManager().Access(_uiTarget);
    const vk::Image swapChainImage = _swapChain.GetImage(scene.targetSwapChainImageIndex);
    util::TransitionImageLayout(commandBuffer, swapChainImage, _swapChain.GetFormat(), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);
    util::TransitionImageLayout(commandBuffer, uiTarget->image, uiTarget->format, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

    util::CopyImageToImage(commandBuffer, uiTarget->image, _swapChain.GetImage(scene.targetSwapChainImageIndex), vk::Extent2D { uiTarget->width, uiTarget->height }, _swapChain.GetExtent());

    util::TransitionImageLayout(commandBuffer, uiTarget->image, uiTarget->format, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, 1);
    util::TransitionImageLayout(commandBuffer, swapChainImage, _swapChain.GetFormat(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal, 1, 0, 1);

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex),
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } },
    };

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Depth())->view,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 } },
    };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
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

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_vertexBuffer);
    const std::array<vk::DeviceSize, 1> offsets = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, offsets.data());

    uint32_t vertexCount = static_cast<uint32_t>(_linesData.size());
    commandBuffer.draw(vertexCount, 1, 0, 0);

    _context->GetDrawStats().Draw(vertexCount);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());

    ClearLines();
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

    _vertexBuffer = _context->Resources()->BufferResourceManager().Create(vertexBufferCreation);
}

void DebugPipeline::UpdateVertexData()
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_vertexBuffer);
    std::memcpy(buffer->mappedPtr, _linesData.data(), _linesData.size() * sizeof(glm::vec3));
}
