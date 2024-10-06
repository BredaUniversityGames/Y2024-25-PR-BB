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
    gBufferCreation
        .SetFormat(GBufferFormat())
        .SetSize(_size.x, _size.y)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    gBufferCreation.SetName("AlbedoM");
    _albedoM = _brain.GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("NormalR");
    _normalR = _brain.GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("EmissiveAO");
    _emissiveAO = _brain.GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetName("Position");
    _position = _brain.GetImageResourceManager().Create(gBufferCreation);

    const Image* albedoMImage = _brain.GetImageResourceManager().Access(_albedoM);
    const Image* normalRImage = _brain.GetImageResourceManager().Access(_normalR);
    const Image* emissiveAOImage = _brain.GetImageResourceManager().Access(_emissiveAO);
    const Image* positionImage = _brain.GetImageResourceManager().Access(_position);

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
    _depthImage = _brain.GetImageResourceManager().Create(depthCreation);

    const Image* image = _brain.GetImageResourceManager().Access(_depthImage);

    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, image->image, _depthFormat, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CreateShadowMapResources()
{
    vk::SamplerCreateInfo shadowSamplerInfo {};
    shadowSamplerInfo.magFilter = vk::Filter::eLinear;
    shadowSamplerInfo.minFilter = vk::Filter::eLinear;
    shadowSamplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    shadowSamplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    shadowSamplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    shadowSamplerInfo.anisotropyEnable = 1;
    shadowSamplerInfo.maxAnisotropy = 16;
    shadowSamplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    shadowSamplerInfo.unnormalizedCoordinates = 0;
    shadowSamplerInfo.compareEnable = 0;
    shadowSamplerInfo.compareOp = vk::CompareOp::eAlways;
    shadowSamplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    shadowSamplerInfo.mipLodBias = 0.0f;
    shadowSamplerInfo.minLod = 0.0f;
    shadowSamplerInfo.maxLod = static_cast<float>(1);
    shadowSamplerInfo.compareEnable = vk::True;
    shadowSamplerInfo.compareOp = vk::CompareOp::eLessOrEqual;
    _shadowSampler = _brain.device.createSampler(shadowSamplerInfo);

    ImageCreation shadowCreation {};
    shadowCreation
        .SetFormat(_shadowFormat)
        .SetType(ImageType::eShadowMap)
        .SetSize(4096, 4096)
        .SetName("Shadow image")
        .SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
        .SetSampler(_shadowSampler);
    _shadowImage = _brain.GetImageResourceManager().Create(shadowCreation);

    const Image* image = _brain.GetImageResourceManager().Access(_shadowImage);

    vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);
    util::TransitionImageLayout(commandBuffer, image->image, _shadowFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    util::EndSingleTimeCommands(_brain, commandBuffer);
}

void GBuffers::CleanUp()
{
    _brain.GetImageResourceManager().Destroy(_albedoM);
    _brain.GetImageResourceManager().Destroy(_normalR);
    _brain.GetImageResourceManager().Destroy(_emissiveAO);
    _brain.GetImageResourceManager().Destroy(_position);
    _brain.GetImageResourceManager().Destroy(_depthImage);
    _brain.GetImageResourceManager().Destroy(_shadowImage);
    _brain.device.destroy(_shadowSampler);
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
    const Image* albedoMImage = _brain.GetImageResourceManager().Access(_albedoM);
    const Image* normalRImage = _brain.GetImageResourceManager().Access(_normalR);
    const Image* emissiveAOImage = _brain.GetImageResourceManager().Access(_emissiveAO);
    const Image* positionImage = _brain.GetImageResourceManager().Access(_position);

    util::TransitionImageLayout(commandBuffer, albedoMImage->image, albedoMImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, normalRImage->image, normalRImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, emissiveAOImage->image, emissiveAOImage->format, oldLayout, newLayout);
    util::TransitionImageLayout(commandBuffer, positionImage->image, positionImage->format, oldLayout, newLayout);
}