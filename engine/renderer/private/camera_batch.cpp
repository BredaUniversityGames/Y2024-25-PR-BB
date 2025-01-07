#include "camera_batch.hpp"

#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

uint32_t roundUpToPowerOfTwo(uint32_t n)
{
    if (n == 0)
    {
        return 1; // Special case: smallest power of two is 1
    }

    // If n is already a power of two, return it
    if ((n & (n - 1)) == 0)
    {
        return n;
    }

    // Round up to the next power of two
    n--; // Subtract 1 to handle exact powers of two correctly
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

CameraBatch::CameraBatch(const std::shared_ptr<GraphicsContext>& context, const std::string& name, const CameraResource& camera, ResourceHandle<GPUImage> depthImage, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL)
    : _context(context)
    , _camera(camera)
    , _depthImage(depthImage)
{
    BufferCreation drawBufferCreation {
        .size = MAX_INSTANCES * sizeof(DrawIndexedIndirectCommand),
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = name + " draw buffer",
    };

    _drawBuffer = _context->Resources()->BufferResourceManager().Create(drawBufferCreation);

    CreateDrawBufferDescriptorSet(drawDSL);

    BufferCreation redirectBufferCreation {
        .size = sizeof(uint32_t) + MAX_INSTANCES * sizeof(uint32_t),
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = name + " redirect buffer",
    };

    _redirectBuffer = _context->Resources()->BufferResourceManager().Create(redirectBufferCreation);

    BufferCreation visibilityCreation {
        .size = MAX_INSTANCES / 8,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
        .name = name + " Visibility Buffer"
    };
    _visibilityBuffer = _context->Resources()->BufferResourceManager().Create(visibilityCreation);

    {
        vk::DescriptorSetAllocateInfo allocateInfo {
            .descriptorPool = _context->VulkanContext()->DescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &visibilityDSL,
        };
        _visibilityDescriptorSet = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

        const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_visibilityBuffer);

        vk::DescriptorBufferInfo bufferInfo {
            .buffer = buffer->buffer,
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet bufferWrite {
            .dstSet = _visibilityDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &bufferInfo,
        };

        _context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});
    }
    {
        vk::DescriptorSetAllocateInfo allocateInfo {
            .descriptorPool = _context->VulkanContext()->DescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &redirectDSL,
        };
        _redirectBufferDescriptorSet = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

        const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_redirectBuffer);

        vk::DescriptorBufferInfo bufferInfo {
            .buffer = buffer->buffer,
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet bufferWrite {
            .dstSet = _redirectBufferDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &bufferInfo,
        };

        _context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});
    }

    const auto* depthImageAccess = _context->Resources()->ImageResourceManager().Access(_depthImage);

    uint16_t hzbSize = roundUpToPowerOfTwo(std::max(depthImageAccess->width, depthImageAccess->height));
    SamplerCreation samplerCreation {
        .name = name + " HZB Sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .anisotropyEnable = false,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(std::floor(std::log2(hzbSize))),
        .reductionMode = vk::SamplerReductionMode::eMin,
    };
    samplerCreation.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToEdge);

    _hzbSampler = _context->Resources()->SamplerResourceManager().Create(samplerCreation);

    CPUImage hzbImage {
        .initialData = {},
        .width = hzbSize,
        .height = hzbSize,
        .depth = 1,
        .layers = 1,
        .mips = static_cast<uint8_t>(std::log2(hzbSize)),
        .flags = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
        .isHDR = false,
        .format = vk::Format::eR16Sfloat,
        .type = ImageType::eDepth, // TODO: should probably be color
        .name = name + " HZB Image",
    };
    _hzbImage = _context->Resources()->ImageResourceManager().Create(hzbImage, _hzbSampler);
}

CameraBatch::~CameraBatch() = default;

void CameraBatch::CreateDrawBufferDescriptorSet(vk::DescriptorSetLayout drawDSL)
{
    vk::DescriptorSetLayout layout = drawDSL;
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    _drawBufferDescriptorSet = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _drawBufferDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});
}
