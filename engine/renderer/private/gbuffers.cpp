#include "gbuffers.hpp"
#include "vulkan_context.hpp"

GBuffers::GBuffers(const std::shared_ptr<VulkanContext>& context, glm::uvec2 size)
    : _context(context)
    , _size(size)
{
    auto supportedDepthFormat = util::FindSupportedFormat(_context->PhysicalDevice(), { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
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
        .SetSize(_size.x, _size.y)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    gBufferCreation.SetFormat(vk::Format::eR8G8B8A8Unorm).SetName("Albedo Metallic");
    _attachments[0] = _context->GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetFormat(vk::Format::eR16G16B16A16Sfloat).SetName("Normal Roughness");
    _attachments[1] = _context->GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetFormat(vk::Format::eR8G8B8A8Unorm).SetName("Emissive AO");
    _attachments[2] = _context->GetImageResourceManager().Create(gBufferCreation);

    gBufferCreation.SetFormat(vk::Format::eR16G16B16A16Sfloat).SetName("Position");
    _attachments[3] = _context->GetImageResourceManager().Create(gBufferCreation);
}

void GBuffers::CreateDepthResources()
{
    ImageCreation depthCreation {};
    depthCreation.SetFormat(_depthFormat).SetSize(_size.x, _size.y).SetName("Depth image").SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    _depthImage = _context->GetImageResourceManager().Create(depthCreation);
}

void GBuffers::CreateShadowMapResources()
{
    vk::SamplerCreateInfo shadowSamplerInfo {};
    shadowSamplerInfo.magFilter = vk::Filter::eLinear;
    shadowSamplerInfo.minFilter = vk::Filter::eLinear;
    shadowSamplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
    shadowSamplerInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
    shadowSamplerInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
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
    _shadowSampler = _context->Device().createSampler(shadowSamplerInfo);

    ImageCreation shadowCreation {};
    shadowCreation
        .SetFormat(_shadowFormat)
        .SetType(ImageType::eShadowMap)
        .SetSize(4096, 4096)
        .SetName("Shadow image")
        .SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
        .SetSampler(_shadowSampler);
    _shadowImage = _context->GetImageResourceManager().Create(shadowCreation);
}

void GBuffers::CleanUp()
{
    for (const auto& attachment : _attachments)
    {
        _context->GetImageResourceManager().Destroy(attachment);
    }
    _context->GetImageResourceManager().Destroy(_depthImage);
    _context->GetImageResourceManager().Destroy(_shadowImage);
    _context->Device().destroy(_shadowSampler);
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
    for (auto attachment : _attachments)
    {
        const Image* image = _context->GetImageResourceManager().Access(attachment);

        util::TransitionImageLayout(commandBuffer, image->image, image->format, oldLayout, newLayout);
    }
}