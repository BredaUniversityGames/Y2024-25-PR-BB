#include "camera_batch.hpp"

#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "math_util.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

CameraBatch::Draw::Draw(const std::shared_ptr<GraphicsContext>& context, const std::string& name, uint32_t instanceCount, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL)
{
    BufferCreation drawBufferCreation {
        .size = instanceCount * sizeof(DrawIndexedIndirectCommand),
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = name + " draw buffer",
    };

    BufferCreation redirectBufferCreation {
        .size = sizeof(uint32_t) + instanceCount * sizeof(uint32_t),
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = name + " redirect buffer",
    };

    BufferCreation visibilityCreation {
        .size = instanceCount / 8,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
        .name = name + " visibility buffer"
    };

    drawBuffer = context->Resources()->BufferResourceManager().Create(drawBufferCreation);
    redirectBuffer = context->Resources()->BufferResourceManager().Create(redirectBufferCreation);
    visibilityBuffer = context->Resources()->BufferResourceManager().Create(visibilityCreation);

    drawDescriptor = CreateDescriptor(context, drawDSL, drawBuffer);
    redirectDescriptor = CreateDescriptor(context, redirectDSL, redirectBuffer);
    visibilityDescriptor = CreateDescriptor(context, visibilityDSL, visibilityBuffer);
}

vk::DescriptorSet CameraBatch::Draw::CreateDescriptor(const std::shared_ptr<GraphicsContext>& context, vk::DescriptorSetLayout dsl, ResourceHandle<Buffer> buffer)
{
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &dsl,
    };
    vk::DescriptorSet descriptor = context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = context->Resources()->BufferResourceManager().Access(buffer)->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = descriptor,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});

    return descriptor;
}

CameraBatch::CameraBatch(const std::shared_ptr<GraphicsContext>& context, const std::string& name, const CameraResource& camera, ResourceHandle<GPUImage> depthImage, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL)
    : _context(context)
    , _camera(camera)
    , _depthImage(depthImage)
    , _staticDraw(context, "Static " + name, MAX_STATIC_INSTANCES, drawDSL, visibilityDSL, redirectDSL)
    , _skinnedDraw(context, "Skinned" + name, MAX_SKINNED_INSTANCES, drawDSL, visibilityDSL, redirectDSL)
{
    const auto* depthImageAccess = _context->Resources()->ImageResourceManager().Access(_depthImage);

    uint16_t hzbSize = math::RoundUpToPowerOfTwo(std::max(depthImageAccess->width, depthImageAccess->height));
    SamplerCreation samplerCreation {
        .name = name + " HZB Sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .anisotropyEnable = false,
        .borderColor = vk::BorderColor::eFloatOpaqueBlack,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(std::floor(std::log2(hzbSize))),
        .reductionMode = _camera.UsesReverseZ() ? vk::SamplerReductionMode::eMin : vk::SamplerReductionMode::eMax,
    };
    samplerCreation.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToBorder);

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
        .format = vk::Format::eR32Sfloat,
        .type = ImageType::e2D,
        .name = name + " HZB Image",
    };
    _hzbImage = _context->Resources()->ImageResourceManager().Create(hzbImage, _hzbSampler);
}

CameraBatch::~CameraBatch() = default;
