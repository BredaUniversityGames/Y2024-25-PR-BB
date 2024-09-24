#include "gbuffers.hpp"

GBuffers::GBuffers(const VulkanBrain& brain, glm::uvec2 size)
    : _brain(brain)
    , _size(size)
{
    auto supportedDepthFormat = util::FindSupportedFormat(_brain.physicalDevice, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    assert(supportedDepthFormat.has_value() && "No supported depth format!");

    _depthFormat = supportedDepthFormat.value();
    _shadowFormat = supportedDepthFormat.value();

    CreateGBuffers();
    CreateDepthResources();
    CreateShadowMapResources();
    CreateViewportAndScissor();
}

GBuffers::~GBuffers()
{
    CleanUp();
}

void GBuffers::Resize(glm::uvec2 size)
{
    if (size == _size)
        return;

    CleanUp();

    _size = size;

    CreateGBuffers();
    CreateDepthResources();
    CreateViewportAndScissor();
}

void GBuffers::CreateGBuffers()
{
    ImageCreation gBufferCreation {};
    gBufferCreation.SetFormat(GBufferFormat()).SetSize(_size.x, _size.y).SetName("GBuffer array").SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
    gBufferCreation.layers = DEFERRED_ATTACHMENT_COUNT;
    _gBuffersImage = _brain.ImageResourceManager().Create(gBufferCreation);

    const Image* image = _brain.ImageResourceManager().Access(_gBuffersImage);

    vk::CommandBuffer cb = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(cb, image->image, image->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, image->layers);
    util::EndSingleTimeCommands(_brain, cb);
}

void GBuffers::CreateDepthResources()
{
    ImageCreation depthCreation {};
    depthCreation.SetFormat(_depthFormat).SetSize(_size.x, _size.y).SetName("Depth image").SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    _depthImage = _brain.ImageResourceManager().Create(depthCreation);

    const Image* image = _brain.ImageResourceManager().Access(_depthImage);

    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, image->image, _depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CreateShadowMapResources()
{
    util::CreateImage(_brain.vmaAllocator,4096 ,4096,_shadowFormat,vk::ImageTiling::eOptimal,
                      vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                      _shadowImage, _shadowImageAllocation, "Shadow image", vk::True, VMA_MEMORY_USAGE_GPU_ONLY);

    _shadowImageView = util::CreateImageView(_brain.device, _shadowImage, _shadowFormat, vk::ImageAspectFlagBits::eDepth);
    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, _shadowImage, _shadowFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CleanUp()
{
    _brain.ImageResourceManager().Destroy(_gBuffersImage);
    _brain.ImageResourceManager().Destroy(_depthImage);
    _brain.device.destroy(_shadowImageView);
    vmaDestroyImage(_brain.vmaAllocator, _shadowImage, _shadowImageAllocation);
}

void GBuffers::CreateViewportAndScissor()
{
    _viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(_size.x), static_cast<float>(_size.y), 0.0f,
        1.0f };
    vk::Extent2D extent { _size.x, _size.y };

    _scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, extent };
}
