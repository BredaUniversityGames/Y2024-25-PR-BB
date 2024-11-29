#include "gbuffers.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

GBuffers::GBuffers(const std::shared_ptr<GraphicsContext>& context, glm::uvec2 size)
    : _context(context)
    , _size(size)
{
    auto supportedDepthFormat = util::FindSupportedFormat(_context->VulkanContext()->PhysicalDevice(), { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    if (supportedDepthFormat.has_value())
    {
        _depthFormat = supportedDepthFormat.value();
        _shadowFormat = supportedDepthFormat.value();
    }
    else
    {
        assert(false && "No supported depth format!");
    }

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
    auto resources { _context->Resources() };

    CPUImage imageData {};
    imageData
        .SetSize(_size.x, _size.y)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    imageData.SetFormat(vk::Format::eR8G8B8A8Unorm).SetName("Albedo Metallic");
    _attachments[0] = resources->ImageResourceManager().Create(imageData);

    imageData.SetFormat(vk::Format::eR16G16B16A16Sfloat).SetName("Normal Roughness");
    _attachments[1] = resources->ImageResourceManager().Create(imageData);

    imageData.SetFormat(vk::Format::eR8G8B8A8Unorm).SetName("Emissive AO");
    _attachments[2] = resources->ImageResourceManager().Create(imageData);

    imageData.SetFormat(vk::Format::eR16G16B16A16Sfloat).SetName("Position");
    _attachments[3] = resources->ImageResourceManager().Create(imageData);
}

void GBuffers::CreateDepthResources()
{
    CPUImage depthImageData {};
    depthImageData.SetFormat(_depthFormat).SetType(ImageType::eDepth).SetSize(_size.x, _size.y).SetName("Depth image").SetFlags(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment);
    _depthImage = _context->Resources()->ImageResourceManager().Create(depthImageData);
}

void GBuffers::CreateShadowMapResources()
{
    SamplerCreation shadowSamplerInfo {
        .name = "Shadow sampler",
        .compareEnable = true,
        .compareOp = vk::CompareOp::eLessOrEqual,
    };
    shadowSamplerInfo.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToEdge);
    _shadowSampler = _context->Resources()->SamplerResourceManager().Create(shadowSamplerInfo);

    CPUImage shadowCreation {};
    shadowCreation
        .SetFormat(_shadowFormat)
        .SetType(ImageType::eShadowMap)
        .SetSize(4096, 4096)
        .SetName("Shadow image")
        .SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
    _shadowImage = _context->Resources()->ImageResourceManager().Create(shadowCreation, _shadowSampler);
}

void GBuffers::CleanUp()
{
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
    for (const auto& attachment : _attachments)
    {
        const GPUImage* image = _context->Resources()->ImageResourceManager().Access(attachment);

        util::TransitionImageLayout(commandBuffer, image->image, image->format, oldLayout, newLayout);
    }
}