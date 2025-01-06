#include "pipelines/presentation_pipeline.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "swap_chain.hpp"
#include "vulkan_helper.hpp"

PresentationPipeline::PresentationPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain, ResourceHandle<GPUImage> input)
    : _context(context)
    , _swapChain(swapChain)
    , _input(input)
{
}

void PresentationPipeline::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Presentation Pipeline");

    const vk::Image swapChainImage = _swapChain.GetImage(scene.targetSwapChainImageIndex);
    const GPUImage* inputImage = _context->Resources()->ImageResourceManager().Access(_input);

    util::TransitionImageLayout(commandBuffer, swapChainImage, _swapChain.GetFormat(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);
    util::TransitionImageLayout(commandBuffer, inputImage->image, inputImage->format,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

    util::CopyImageToImage(commandBuffer, inputImage->image, _swapChain.GetImage(scene.targetSwapChainImageIndex), vk::Extent2D { inputImage->width, inputImage->height }, _swapChain.GetExtent());

    util::TransitionImageLayout(commandBuffer, swapChainImage, _swapChain.GetFormat(),
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);
}