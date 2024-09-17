#include "gbuffers.hpp"

GBuffers::GBuffers(const VulkanBrain& brain, glm::uvec2 size) :
        _brain(brain),
        _size(size)
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
    if(size == _size)
        return;

    CleanUp();

    _size = size;

    CreateGBuffers();
    CreateDepthResources();
    CreateViewportAndScissor();
}

void GBuffers::CreateGBuffers()
{
    auto format = GBufferFormat();
    util::CreateImage(_brain.vmaAllocator, _size.x, _size.y, format,
                      vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                      _gBuffersImageArray, _gBufferAllocation, "GBuffer array", false, VMA_MEMORY_USAGE_GPU_ONLY, DEFERRED_ATTACHMENT_COUNT);
    util::NameObject(_gBuffersImageArray, "[IMAGE] GBuffer Array", _brain.device, _brain.dldi);

    for(size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        _gBufferViews[i] = util::CreateImageView(_brain.device, _gBuffersImageArray, format, vk::ImageAspectFlagBits::eColor, i);
        util::NameObject(_gBufferViews[i], _names[i], _brain.device, _brain.dldi);
    }

    vk::CommandBuffer cb = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(cb, _gBuffersImageArray, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, DEFERRED_ATTACHMENT_COUNT);
    util::EndSingleTimeCommands(_brain, cb);
}

void GBuffers::CreateDepthResources()
{
    util::CreateImage(_brain.vmaAllocator, _size.x, _size.y,
                      _depthFormat, vk::ImageTiling::eOptimal,
                      vk::ImageUsageFlagBits::eDepthStencilAttachment,
                      _depthImage, _depthImageAllocation, "Depth image", false, VMA_MEMORY_USAGE_GPU_ONLY);

    _depthImageView = util::CreateImageView(_brain.device, _depthImage, _depthFormat, vk::ImageAspectFlagBits::eDepth);

    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, _depthImage, _depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CreateShadowMapResources()
{
    util::CreateImage(_brain.vmaAllocator,1024,1024,_shadowFormat,vk::ImageTiling::eOptimal,
                      vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                      _shadowImage, _shadowImageAllocation, "Shadow image", false, VMA_MEMORY_USAGE_GPU_ONLY);

    _shadowImageView = util::CreateImageView(_brain.device, _shadowImage, _shadowFormat, vk::ImageAspectFlagBits::eDepth);
    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, _shadowImage, _shadowFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CleanUp()
{
    vmaDestroyImage(_brain.vmaAllocator, _gBuffersImageArray, _gBufferAllocation);
    for(size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        _brain.device.destroy(_gBufferViews[i]);

    _brain.device.destroy(_depthImageView);
    vmaDestroyImage(_brain.vmaAllocator, _depthImage, _depthImageAllocation);

    _brain.device.destroy(_shadowImageView);
    vmaDestroyImage(_brain.vmaAllocator, _shadowImage, _shadowImageAllocation);
}

void GBuffers::CreateViewportAndScissor()
{
    _viewport = vk::Viewport{ 0.0f, 0.0f, static_cast<float>(_size.x), static_cast<float>(_size.y), 0.0f,
                              1.0f };
    vk::Extent2D extent{ _size.x, _size.y };

    _scissor = vk::Rect2D{ vk::Offset2D{ 0, 0 }, extent };
}
