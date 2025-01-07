#include "pipelines/cluster_generation_pipeline.hpp"

#include "gpu_scene.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

ClusterGenerationPipeline::ClusterGenerationPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const SwapChain& swapChain, ResourceHandle<Buffer>& outputBuffer)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
    , _swapChain(swapChain)
    , _outputBuffer(outputBuffer)
{
    CreateDescriptorSet();
    CreatePipeline();
}

ClusterGenerationPipeline::~ClusterGenerationPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);

    _context->VulkanContext()->Device().destroy(_outputBufferDescriptorSetLayout);
}

void ClusterGenerationPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Cluster AABB Generation");
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    _pushConstants.screenSize = glm::vec2(_swapChain.GetExtent().width, _swapChain.GetExtent().height);
    _numTilesX = static_cast<uint32_t>(std::ceil(_pushConstants.screenSize.x / _clusterSizeX));
    _pushConstants.tileSizes = glm::uvec4(_clusterSizeX, _clusterSizeY, _clusterSizeZ, _numTilesX);
    _pushConstants.normPerTileSize = glm::vec2(1.0f / _pushConstants.tileSizes.x, 1.0f / _pushConstants.tileSizes.y);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, _pushConstants);

    // TODO: Bind Writing buffer.
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, { _outputBufferDescriptorSet }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});

    commandBuffer.dispatch(_clusterSizeX, _clusterSizeY, _clusterSizeZ);
}

void ClusterGenerationPipeline::CreatePipeline()
{
    vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    std::array<vk::DescriptorSetLayout, 2> layouts {
        _outputBufferDescriptorSetLayout,
        CameraResource::DescriptorSetLayout(),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout), "Failed to create pipeline layout for clustering pipeline!");

    std::vector<std::byte> compShader = shader::ReadFile("shaders/bin/cluster_aabb_generation.comp.spv");
    vk::ShaderModule computeModule = shader::CreateShaderModule(compShader, _context->VulkanContext()->Device());

    vk::PipelineShaderStageCreateInfo computeStage {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = computeModule,
        .pName = "main",
    };

    vk::ComputePipelineCreateInfo pipelineCreateInfo {
        .stage = computeStage,
        .layout = _pipelineLayout,
    };

    // PipelineBuilder reflector { _context };
    // reflector.AddShaderStage(vk::ShaderStageFlagBits::eCompute, compShader);
    // reflector.BuildPipeline(_pipeline, _pipelineLayout);

    auto result = _context->VulkanContext()->Device().createComputePipeline(nullptr, pipelineCreateInfo, nullptr);
    _pipeline = result.value;
    _context->VulkanContext()->Device().destroy(computeModule);
}

void ClusterGenerationPipeline::CreateDescriptorSet()
{
    vk::DescriptorSetLayoutBinding layoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
    };

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &layoutBinding,
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_outputBufferDescriptorSetLayout), "Failed creating descriptor set layout!");

    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_outputBufferDescriptorSetLayout,
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, &_outputBufferDescriptorSet), "Failed to allocate descriptor set for clustering pipeline!");

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_outputBuffer);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _outputBufferDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
}
