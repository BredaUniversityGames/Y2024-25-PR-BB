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

    CreateGBuffers();
    CreateDepthResources();
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
    gBufferCreation
        .SetFormat(GBufferFormat())
        .SetSize(_size.x, _size.y)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    gBufferCreation.SetName("AlbedoM");
    _albedoM = _brain.ImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("NormalR");
    _normalR = _brain.ImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("EmissiveAO");
    _emissiveAO = _brain.ImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("Position");
    _position = _brain.ImageResourceManager().Create(gBufferCreation);

    const Image* albedoMImage = _brain.ImageResourceManager().Access(_albedoM);
    const Image* normalRImage = _brain.ImageResourceManager().Access(_normalR);
    const Image* emissiveAOImage = _brain.ImageResourceManager().Access(_emissiveAO);
    const Image* positionImage = _brain.ImageResourceManager().Access(_position);

    vk::CommandBuffer cb = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(cb, albedoMImage->image, albedoMImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(cb, normalRImage->image, normalRImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(cb, emissiveAOImage->image, emissiveAOImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(cb, positionImage->image, positionImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);

    util::EndSingleTimeCommands(_brain, cb);
}

void GBuffers::CreateDepthResources()
{

    ImageCreation depthCreation {};

    depthCreation.SetFormat(_depthFormat).SetSize(_size.x, _size.y).SetName("Depth image").SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    _depthImage = _brain.ImageResourceManager().Create(depthCreation);

    const Image* image = _brain.ImageResourceManager().Access(_depthImage);

    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);

    util::TransitionImageLayout(commandBuffer, image->image, _depthFormat, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CleanUp()
{

    _brain.ImageResourceManager().Destroy(_albedoM);
    _brain.ImageResourceManager().Destroy(_normalR);
    _brain.ImageResourceManager().Destroy(_emissiveAO);
    _brain.ImageResourceManager().Destroy(_position);
    _brain.ImageResourceManager().Destroy(_depthImage);
}

void GBuffers::CreateViewportAndScissor()
{
    _viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(_size.x), static_cast<float>(_size.y), 0.0f,
        1.0f };
    vk::Extent2D extent { _size.x, _size.y };

    _scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, extent };
}

void GBuffers::TransitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    const Image* albedoMImage = _brain.ImageResourceManager().Access(_albedoM);
    const Image* normalRImage = _brain.ImageResourceManager().Access(_normalR);
    const Image* emissiveAOImage = _brain.ImageResourceManager().Access(_emissiveAO);
    const Image* positionImage = _brain.ImageResourceManager().Access(_position);

    util::TransitionImageLayout(commandBuffer, albedoMImage->image, albedoMImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, normalRImage->image, normalRImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, emissiveAOImage->image, emissiveAOImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, positionImage->image, positionImage->format, oldLayout, newLayout);
}