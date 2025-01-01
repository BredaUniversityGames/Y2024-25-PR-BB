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

CameraBatch::CameraBatch(const std::shared_ptr<GraphicsContext>& context, const CameraResource& camera, ResourceHandle<GPUImage> depthImage, ResourceHandle<Buffer> drawBuffer, vk::DescriptorSetLayout drawDSL, glm::uvec2 displaySize)
    : _context(context)
    , _camera(camera)
    , _depthImage(depthImage)
{
    const auto* mainDrawBuffer = _context->Resources()->BufferResourceManager().Access(drawBuffer);

    BufferCreation creation {
        .size = mainDrawBuffer->size,
        .usage = mainDrawBuffer->usage,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = "Geometry draw buffer",
    };

    _drawBuffer = _context->Resources()->BufferResourceManager().Create(creation);
    CreateDrawBufferDescriptorSet(drawDSL);

    BufferCreation visibilityCreation {
        .size = sizeof(bool) * MAX_INSTANCES, // TODO: Can be packed into multiple bits
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
        .name = "Visibility Buffer"
    };
    _visibilityBuffer = _context->Resources()->BufferResourceManager().Create(visibilityCreation);

    BufferCreation orderingBufferCreation {
        .size = 1,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    };
    for (size_t i = 0; i < _orderingBuffers.size(); ++i)
    {
        orderingBufferCreation.name = "Ordering buffer " + std::to_string(i);
        _orderingBuffers[i] = _context->Resources()->BufferResourceManager().Create(orderingBufferCreation);
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics }
    };
    std::vector<std::string_view> names { "VisibilityBuffer" };

    _visibilityDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_visibilityDSL,
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

    uint16_t hzbSize = std::max(displaySize.x, displaySize.y);
    SamplerCreation samplerCreation {
        .name = "HZB Sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .anisotropyEnable = false,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .minLod = 0.0f,
        .maxLod = std::floorf(std::log2f(hzbSize)),
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
        .name = "HZB Image",
    };
    _hzbImage = _context->Resources()->ImageResourceManager().Create(hzbImage, _hzbSampler);
}

CameraBatch::~CameraBatch()
{
    const auto& vkContext = _context->VulkanContext();

    vkContext->Device().destroy(_visibilityDSL);
}

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
