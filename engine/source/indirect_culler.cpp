#include <shaders/shader_loader.hpp>
#include "indirect_culler.hpp"

#include "vulkan_helper.hpp"
#include "gpu_scene.hpp"
#include "gpu_resources.hpp"

IndirectCuller::IndirectCuller(const VulkanBrain& brain, const GPUScene& gpuScene)
    : _brain(brain)
{
    CreateCullingPipeline(gpuScene);
}

IndirectCuller::~IndirectCuller()
{
    _brain.device.destroy(_cullingPipeline);
    _brain.device.destroy(_cullingPipelineLayout);
}

void IndirectCuller::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const CameraResource& camera)
{
    const uint32_t localSize = 64;
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 0, { scene.gpuScene.DrawBufferDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 1, { scene.gpuScene.GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 2, { camera.DescriptorSet(currentFrame) }, {});
    commandBuffer.dispatch((scene.gpuScene.DrawCount() + localSize - 1) / localSize, 1, 1);

    vk::Buffer indirectDrawBuffer = _brain.GetBufferResourceManager().Access(scene.gpuScene.IndirectDrawBuffer(currentFrame))->buffer;

    vk::BufferMemoryBarrier bufferMemoryBarrier {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = indirectDrawBuffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, { bufferMemoryBarrier }, {});
}

void IndirectCuller::CreateCullingPipeline(const GPUScene& gpuScene)
{
    std::array<vk::DescriptorSetLayout, 3> layouts {
        gpuScene.DrawBufferLayout(),
        gpuScene.GetObjectInstancesDescriptorSetLayout(),
        CameraResource::DescriptorSetLayout(),
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
    };

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_cullingPipelineLayout), "Failed creating culling pipeline layout!");

    vk::ShaderModule computeModule = shader::CreateShaderModule(shader::ReadFile("shaders/bin/culling.comp.spv"), _brain.device);

    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo;
    shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
    shaderStageCreateInfo.module = computeModule;
    shaderStageCreateInfo.pName = "main";

    vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
    computePipelineCreateInfo.layout = _cullingPipelineLayout;
    computePipelineCreateInfo.stage = shaderStageCreateInfo;

    auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);

    _cullingPipeline = result.value;

    _brain.device.destroy(computeModule);
}
