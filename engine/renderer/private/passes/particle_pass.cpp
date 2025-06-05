#include "passes/particle_pass.hpp"

#include "bloom_settings.hpp"
#include "camera.hpp"
#include "ecs_module.hpp"
#include "emitter_component.hpp"
#include "glm/glm.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <pipeline_builder.hpp>
#include <random>

ParticlePass::ParticlePass(const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings)
    : _context(context)
    , _ecs(ecs)
    , _gBuffers(gBuffers)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _bloomSettings(bloomSettings)
{
    srand(time(0));

    CreateDescriptorSetLayouts();
    CreateBuffers();
    CreateDescriptorSets();
    CreatePipelines();
}

ParticlePass::~ParticlePass()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    // Pipeline stuff
    for (auto& pipeline : _pipelines)
    {
        vkContext->Device().destroy(pipeline);
    }
    for (auto& layout : _pipelineLayouts)
    {
        vkContext->Device().destroy(layout);
    }
    // Buffer stuff
    for (auto& storageBuffer : _particlesBuffers)
    {
        resources->BufferResourceManager().Destroy(storageBuffer);
    }
    resources->BufferResourceManager().Destroy(_drawCommandsBuffer);
    resources->BufferResourceManager().Destroy(_culledInstancesBuffer);
    resources->BufferResourceManager().Destroy(_localEmittersBuffer);
    resources->BufferResourceManager().Destroy(_emittersBuffer);
    resources->BufferResourceManager().Destroy(_vertexBuffer);
    resources->BufferResourceManager().Destroy(_indexBuffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::vmaDestroyBuffer(vkContext->MemoryAllocator(), _emitterStagingBuffer[i], _emitterStagingBufferAllocation[i]);
        util::vmaDestroyBuffer(vkContext->MemoryAllocator(), _localEmitterStagingBuffer[i], _localEmitterStagingBufferAllocation[i]);
    }

    vkContext->Device().destroy(_localEmittersDescriptorSetLayout);
    vkContext->Device().destroy(_particlesBuffersDescriptorSetLayout);
    vkContext->Device().destroy(_emittersBufferDescriptorSetLayout);
    vkContext->Device().destroy(_culledInstancesDescriptorSetLayout);
    vkContext->Device().destroy(_drawCommandsDescriptorSetLayout);
}

void ParticlePass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Particle Pass");

    UpdateEmitters(commandBuffer, currentFrame);

    RecordKickOff(commandBuffer);

    if (!_emitters.empty())
    {
        RecordEmit(commandBuffer);
    }

    RecordSimulate(commandBuffer, scene.gpuScene->MainCamera(), scene.deltaTime, currentFrame);

    RecordRenderIndexedIndirect(commandBuffer, scene, currentFrame);

    UpdateAliveLists();
}

void ParticlePass::RecordKickOff(vk::CommandBuffer commandBuffer)
{
    auto vkContext { _context->VulkanContext() };

    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, vkContext->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eKickOff)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)], 2, _drawCommandsDescriptorSet, {});

    commandBuffer.dispatch(1, 1, 1);

    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    util::EndLabel(commandBuffer, vkContext->Dldi());
}

void ParticlePass::RecordEmit(vk::CommandBuffer commandBuffer)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    util::BeginLabel(commandBuffer, "Emit particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, vkContext->Dldi());

    // make sure the copy buffer command is done before dispatching
    vk::BufferMemoryBarrier barrier {};
    barrier.buffer = resources->BufferResourceManager().Access(_emittersBuffer)->buffer;
    barrier.size = MAX_EMITTERS * sizeof(Emitter);
    barrier.offset = 0;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, {}, barrier, {});

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eEmit)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], 2, _emittersDescriptorSet, {});

    // spawn as many threads as there's particles to emit
    uint32_t bufferOffset = 0;
    for (bufferOffset = 0; bufferOffset < std::min(static_cast<uint32_t>(_emitters.size()), MAX_EMITTERS); bufferOffset++)
    {
        _emitPushConstant.bufferOffset = bufferOffset;
        commandBuffer.pushConstants<EmitPushConstant>(_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], vk::ShaderStageFlagBits::eCompute, 0, { _emitPushConstant });
        // +63 so we always dispatch at least once.
        commandBuffer.dispatch((_emitters[bufferOffset].count + 63) / 64, 1, 1);
    }
    _context->GetDrawStats().SetEmitterCount(_emitters.size());
    _emitters.clear();

    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    util::EndLabel(commandBuffer, vkContext->Dldi());
}

void ParticlePass::RecordSimulate(vk::CommandBuffer commandBuffer, const CameraResource& camera, float deltaTime, uint32_t currentFrame)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    // make sure the copy buffer command is done before dispatching
    vk::BufferMemoryBarrier barrier {};
    barrier.buffer = resources->BufferResourceManager().Access(_localEmittersBuffer)->buffer;
    barrier.size = MAX_EMITTERS * sizeof(LocalEmitter);
    barrier.offset = 0;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, {}, barrier, {});

    util::BeginLabel(commandBuffer, "Simulate particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, vkContext->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eSimulate)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 2, _culledInstancesDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 3, camera.DescriptorSet(currentFrame), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 4, _localEmittersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 5, _drawCommandsDescriptorSet, {});

    _simulatePushConstant.deltaTime = deltaTime * 1e-3;
    _simulatePushConstant.localEmitterCount = _localEmitters.size();
    commandBuffer.pushConstants<SimulatePushConstant>(_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], vk::ShaderStageFlagBits::eCompute, 0, { _simulatePushConstant });

    commandBuffer.dispatch(MAX_PARTICLES / 256, 1, 1);
    _localEmitters.clear();

    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eMemoryRead;
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    util::EndLabel(commandBuffer, vkContext->Dldi());
}

void ParticlePass::RecordRenderIndexedIndirect(vk::CommandBuffer commandBuffer, const RenderSceneDescription& scene, uint32_t currentFrame)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = resources->ImageResourceManager().Access(_hdrTarget)->view;
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachmentInfos[0].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->view;
    colorAttachmentInfos[1].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[1].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[1].loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = resources->ImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _gBuffers.Size().x, _gBuffers.Size().y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    util::BeginLabel(commandBuffer, "Particle rendering pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, vkContext->Dldi());
    commandBuffer.beginRenderingKHR(&renderingInfo, vkContext->Dldi());

    commandBuffer.setViewport(0, 1, &_gBuffers.Viewport());
    commandBuffer.setScissor(0, 1, &_gBuffers.Scissor());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)], 0, _context->BindlessSet(), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)], 1, _culledInstancesDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)], 2, scene.gpuScene->MainCamera().DescriptorSet(currentFrame), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)], 3, scene.gpuScene->GetSceneDescriptorSet(currentFrame), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)], 4, _bloomSettings.GetDescriptorSetData(currentFrame), {});

    vk::Buffer vertexBuffer = resources->BufferResourceManager().Access(_vertexBuffer)->buffer;
    vk::Buffer indexBuffer = resources->BufferResourceManager().Access(_indexBuffer)->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexedIndirect(resources->BufferResourceManager().Access(_drawCommandsBuffer)->buffer, 0, 1, sizeof(DrawIndexedIndirectCommand));

    _context->GetDrawStats().Draw(6);

    commandBuffer.endRenderingKHR(vkContext->Dldi());
    util::EndLabel(commandBuffer, vkContext->Dldi());
}

void ParticlePass::UpdateEmitters(vk::CommandBuffer commandBuffer, uint32_t currentFrame)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    auto view = _ecs.GetRegistry().view<ParticleEmitterComponent, ActiveEmitterTag>();
    for (auto entity : view)
    {
        auto& component = view.get<ParticleEmitterComponent>(entity);

        // continuous emission
        if (component.currentEmitDelay < 0.0f || component.emitOnce)
        {
            component.emitter.randomValue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            component.emitter.count = component.count;
            _emitters.emplace_back(component.emitter);

            component.currentEmitDelay = component.maxEmitDelay;
        }

        // burst emission
        for (auto it = component.bursts.begin(); it != component.bursts.end();)
        {
            auto copyIt = it;
            it++;
            auto& burst = *copyIt;
            if (burst.currentInterval < 0.0f)
            {
                if (burst.cycles > 0 || burst.loop)
                {
                    component.emitter.randomValue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    component.emitter.count = burst.count;
                    _emitters.emplace_back(component.emitter);

                    if (!burst.loop)
                    {
                        burst.cycles--;
                    }
                    burst.currentInterval = burst.maxInterval;
                }
                else
                {
                    component.bursts.erase(copyIt);
                }
            }
        }

        // remove after emitting once
        if (component.emitOnce)
        {
            _ecs.GetRegistry().remove<ParticleEmitterComponent>(entity);
            _ecs.GetRegistry().remove<ActiveEmitterTag>(entity);
        }

        // add local emitters to vector
        if (HasAnyFlags(static_cast<ParticleRenderFlagBits>(component.emitter.flags), ParticleRenderFlagBits::eIsLocal))
        {
            LocalEmitter localEmitter;
            localEmitter.position = component.emitter.position;
            localEmitter.id = component.emitter.id;

            _localEmitters.emplace_back(localEmitter);
        }
    }

    // copy over local emitters to buffer
    if (!_localEmitters.empty())
    {
        vk::DeviceSize bufferSize = glm::min(static_cast<uint32_t>(_localEmitters.size()), MAX_EMITTERS) * sizeof(LocalEmitter);

        // copy data to staging buffer
        vmaCopyMemoryToAllocation(vkContext->MemoryAllocator(), _localEmitters.data(), _localEmitterStagingBufferAllocation[currentFrame], 0, bufferSize);

        // make sure staging buffer data is complete before copybuffer
        vk::BufferMemoryBarrier barrier {};
        barrier.buffer = _localEmitterStagingBuffer[currentFrame];
        barrier.size = bufferSize;
        barrier.offset = 0;
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags { 0 }, {}, barrier, {});

        // copy staging buffer to buffer
        util::CopyBuffer(commandBuffer, _localEmitterStagingBuffer[currentFrame], resources->BufferResourceManager().Access(_localEmittersBuffer)->buffer, bufferSize);
    }

    // copy over emitters
    if (!_emitters.empty())
    {
        vk::DeviceSize bufferSize = glm::min(static_cast<uint32_t>(_emitters.size()), MAX_EMITTERS) * sizeof(Emitter);

        // copy data to staging buffer
        vmaCopyMemoryToAllocation(vkContext->MemoryAllocator(), _emitters.data(), _emitterStagingBufferAllocation[currentFrame], 0, bufferSize);

        // make sure staging buffer data is complete before copybuffer
        vk::BufferMemoryBarrier barrier {};
        barrier.buffer = _emitterStagingBuffer[currentFrame];
        barrier.size = bufferSize;
        barrier.offset = 0;
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags { 0 }, {}, barrier, {});

        // copy staging buffer to buffer
        util::CopyBuffer(commandBuffer, _emitterStagingBuffer[currentFrame], resources->BufferResourceManager().Access(_emittersBuffer)->buffer, bufferSize);
    }
}

void ParticlePass::UpdateAliveLists()
{
    std::swap(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eAliveNew)], _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eAliveCurrent)]);
    UpdateParticleBuffersDescriptorSets();
}

void ParticlePass::ResetParticles()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    auto cmdBuffer = SingleTimeCommands(_context->VulkanContext());

    std::vector<ParticleCounters> counters(1);
    cmdBuffer.CopyIntoLocalBuffer(counters, 0, resources->BufferResourceManager().Access(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eCounter)])->buffer);

    _emitters.clear();
    _localEmitters.clear();

    cmdBuffer.Submit();
}

void ParticlePass::CreatePipelines()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    { // kick-off
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 3> layouts = { _context->BindlessLayout(), _particlesBuffersDescriptorSetLayout, _drawCommandsDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)] = vkContext->Device().createPipelineLayout(pipelineLayoutCreateInfo);

        std::vector<std::byte> byteCode = shader::ReadFile("shaders/bin/kick_off.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, vkContext->Device());

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = shaderModule,
            .pName = "main",
        };

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {
            .stage = shaderStageCreateInfo,
            .layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)],
        };

        auto result = vkContext->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the kick_off compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eKickOff)] = result.value;

        vkContext->Device().destroy(shaderModule);
    }

    { // emit
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 3> layouts = { _context->BindlessLayout(), _particlesBuffersDescriptorSetLayout, _emittersBufferDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

        vk::PushConstantRange pcRange = {
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 0,
            .size = sizeof(_emitPushConstant),
        };

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

        _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)] = vkContext->Device().createPipelineLayout(pipelineLayoutCreateInfo);

        auto byteCode = shader::ReadFile("shaders/bin/emit.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, vkContext->Device());

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = shaderModule,
            .pName = "main",
        };

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {
            .stage = shaderStageCreateInfo,
            .layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)],
        };

        auto result = vkContext->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the emit compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eEmit)] = result.value;

        vkContext->Device().destroy(shaderModule);
    }

    { // simulate
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 6> layouts = { _context->BindlessLayout(), _particlesBuffersDescriptorSetLayout, _culledInstancesDescriptorSetLayout, CameraResource::DescriptorSetLayout(), _localEmittersDescriptorSetLayout, _drawCommandsDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

        vk::PushConstantRange pcRange = {
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 0,
            .size = sizeof(_simulatePushConstant),
        };

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

        _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)] = vkContext->Device().createPipelineLayout(pipelineLayoutCreateInfo);

        std::vector<std::byte> byteCode = shader::ReadFile("shaders/bin/simulate.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, vkContext->Device());

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = shaderModule,
            .pName = "main",
        };

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {
            .stage = shaderStageCreateInfo,
            .layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)],
        };

        auto result = vkContext->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the simulate compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eSimulate)] = result.value;

        vkContext->Device().destroy(shaderModule);
    }

    { // indexed indirect rendering (billboard)
        std::array<vk::PipelineColorBlendAttachmentState, 2> colorBlendAttachmentState {};
        colorBlendAttachmentState[0].blendEnable = vk::False;
        colorBlendAttachmentState[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        memcpy(&colorBlendAttachmentState[1], &colorBlendAttachmentState[0], sizeof(vk::PipelineColorBlendAttachmentState));

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.logicOpEnable = vk::False;
        colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentState.size();
        colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentState.data();

        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
        depthStencilStateCreateInfo.depthTestEnable = true;
        depthStencilStateCreateInfo.depthWriteEnable = true;
        depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreaterOrEqual;
        depthStencilStateCreateInfo.depthBoundsTestEnable = false;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
        depthStencilStateCreateInfo.stencilTestEnable = false;

        std::vector<vk::Format> formats = {
            resources->ImageResourceManager().Access(_hdrTarget)->format,
            resources->ImageResourceManager().Access(_brightnessTarget)->format
        };

        std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/billboard.vert.spv");
        std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/particle.frag.spv");

        GraphicsPipelineBuilder pipelineBuilder { _context };
        pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
        pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
        auto result = pipelineBuilder
                          .SetColorBlendState(colorBlendStateCreateInfo)
                          .SetColorAttachmentFormats(formats)
                          .SetDepthAttachmentFormat(resources->ImageResourceManager().Access(_gBuffers.Depth())->format)
                          .SetDepthStencilState(depthStencilStateCreateInfo)
                          .BuildPipeline();

        _pipelineLayouts.at(static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)) = std::get<0>(result);
        _pipelines.at(static_cast<uint32_t>(ShaderStages::eRenderIndexedIndirect)) = std::get<1>(result);
    }
}

void ParticlePass::CreateDescriptorSetLayouts()
{
    auto vkContext { _context->VulkanContext() };

    { // Particle Storage Buffers
        std::array<vk::DescriptorSetLayoutBinding, 5> bindings {};
        std::array<vk::DescriptorBindingFlags, 5> flags {};
        for (size_t i = 0; i < bindings.size(); i++)
        {
            vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[i] };
            descriptorSetLayoutBinding.binding = i;
            descriptorSetLayoutBinding.descriptorCount = 1;
            descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

            flags[i] = vk::DescriptorBindingFlagBits::eUpdateAfterBind;
        }

        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingCreateInfo {};
        bindingCreateInfo.bindingCount = bindings.size();
        bindingCreateInfo.pBindingFlags = flags.data();

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();
        createInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
        createInfo.pNext = &bindingCreateInfo;

        util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&createInfo, nullptr, &_particlesBuffersDescriptorSetLayout),
            "Failed creating particle buffers descriptor set layout!");
    }

    { // Emitter Uniform Buffer
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

        vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&createInfo, nullptr, &_emittersBufferDescriptorSetLayout),
            "Failed creating emitter buffer descriptor set layout!");
    }

    { // Local Emitter Uniform Buffer
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

        vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&createInfo, nullptr, &_localEmittersDescriptorSetLayout),
            "Failed creating local emitter buffer descriptor set layout!");
    }

    { // Particle Instances Storage Buffer
        std::vector<vk::DescriptorSetLayoutBinding> bindings {};

        vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings.emplace_back() };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        _culledInstancesDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*vkContext, bindings, { "CulledInstancesSSB" });
    }

    { // Draw Commands buffer
        std::vector<vk::DescriptorSetLayoutBinding> bindings {};

        vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings.emplace_back() };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&createInfo, nullptr, &_drawCommandsDescriptorSetLayout),
            "Failed creating particle draw commands buffer descriptor set layout!");
    }
}

void ParticlePass::CreateDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };

    { // Particle Storage Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = vkContext->DescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_particlesBuffersDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Storage Buffer descriptor sets!");

        _particlesBuffersDescriptorSet = descriptorSets[0];
        UpdateParticleBuffersDescriptorSets();
    }

    { // Culled Instances Storage Buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = vkContext->DescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_culledInstancesDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Instances Storage Buffer descriptor sets!");

        _culledInstancesDescriptorSet = descriptorSets[0];
        UpdateParticleInstancesBufferDescriptorSet();
    }

    { // Emitter Uniform Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = vkContext->DescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_emittersBufferDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Emitter Uniform Buffer descriptor sets!");

        _emittersDescriptorSet = descriptorSets[0];
        UpdateEmittersBuffersDescriptorSets();
    }

    { // Local Emitter Uniform Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = vkContext->DescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_localEmittersDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Local Emitter Uniform Buffer descriptor sets!");

        _localEmittersDescriptorSet = descriptorSets[0];
        UpdateLocalEmittersBuffersDescriptorSets();
    }

    { // Draw Commands buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = vkContext->DescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_drawCommandsDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Draw Commands Buffer descriptor set!");

        _drawCommandsDescriptorSet = descriptorSets[0];
        UpdateDrawCommandsBufferDescriptorSet();
    }
}

void ParticlePass::UpdateParticleBuffersDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 5> descriptorWrites {};

    // Particle SSB (binding = 0)
    uint32_t index = static_cast<uint32_t>(ParticleBufferUsage::eParticle);
    vk::DescriptorBufferInfo particleBufferInfo {};
    particleBufferInfo.buffer = resources->BufferResourceManager().Access(_particlesBuffers[index])->buffer;
    particleBufferInfo.offset = 0;
    particleBufferInfo.range = sizeof(Particle) * MAX_PARTICLES;
    vk::WriteDescriptorSet& particleBufferWrite { descriptorWrites[index] };
    particleBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    particleBufferWrite.dstBinding = index;
    particleBufferWrite.dstArrayElement = 0;
    particleBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    particleBufferWrite.descriptorCount = 1;
    particleBufferWrite.pBufferInfo = &particleBufferInfo;

    // Alive NEW list SSB (binding = 1)
    index = static_cast<uint32_t>(ParticleBufferUsage::eAliveNew);
    vk::DescriptorBufferInfo aliveNEWBufferInfo {};
    aliveNEWBufferInfo.buffer = resources->BufferResourceManager().Access(_particlesBuffers[index])->buffer;
    aliveNEWBufferInfo.offset = 0;
    aliveNEWBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveNEWBufferWrite { descriptorWrites[index] };
    aliveNEWBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    aliveNEWBufferWrite.dstBinding = index;
    aliveNEWBufferWrite.dstArrayElement = 0;
    aliveNEWBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveNEWBufferWrite.descriptorCount = 1;
    aliveNEWBufferWrite.pBufferInfo = &aliveNEWBufferInfo;

    // Alive CURRENT list SSB (binding = 2)
    index = static_cast<uint32_t>(ParticleBufferUsage::eAliveCurrent);
    vk::DescriptorBufferInfo aliveCURRENTBufferInfo {};
    aliveCURRENTBufferInfo.buffer = resources->BufferResourceManager().Access(_particlesBuffers[index])->buffer;
    aliveCURRENTBufferInfo.offset = 0;
    aliveCURRENTBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveCURRENTBufferWrite { descriptorWrites[index] };
    aliveCURRENTBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    aliveCURRENTBufferWrite.dstBinding = index;
    aliveCURRENTBufferWrite.dstArrayElement = 0;
    aliveCURRENTBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveCURRENTBufferWrite.descriptorCount = 1;
    aliveCURRENTBufferWrite.pBufferInfo = &aliveCURRENTBufferInfo;

    // Dead list SSB (binding = 3)
    index = static_cast<uint32_t>(ParticleBufferUsage::eDead);
    vk::DescriptorBufferInfo deadBufferInfo {};
    deadBufferInfo.buffer = resources->BufferResourceManager().Access(_particlesBuffers[index])->buffer;
    deadBufferInfo.offset = 0;
    deadBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& deadBufferWrite { descriptorWrites[index] };
    deadBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    deadBufferWrite.dstBinding = index;
    deadBufferWrite.dstArrayElement = 0;
    deadBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    deadBufferWrite.descriptorCount = 1;
    deadBufferWrite.pBufferInfo = &deadBufferInfo;

    // Counter SSB (binding = 4)
    index = static_cast<uint32_t>(ParticleBufferUsage::eCounter);
    vk::DescriptorBufferInfo counterBufferInfo {};
    counterBufferInfo.buffer = resources->BufferResourceManager().Access(_particlesBuffers[index])->buffer;
    counterBufferInfo.offset = 0;
    counterBufferInfo.range = sizeof(ParticleCounters);
    vk::WriteDescriptorSet& counterBufferWrite { descriptorWrites[index] };
    counterBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    counterBufferWrite.dstBinding = index;
    counterBufferWrite.dstArrayElement = 0;
    counterBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    counterBufferWrite.descriptorCount = 1;
    counterBufferWrite.pBufferInfo = &counterBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePass::UpdateParticleInstancesBufferDescriptorSet()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    // Culled Instance (binding = 0)
    vk::DescriptorBufferInfo culledInstancesBufferInfo {};
    culledInstancesBufferInfo.buffer = resources->BufferResourceManager().Access(_culledInstancesBuffer)->buffer;
    culledInstancesBufferInfo.offset = 0;
    culledInstancesBufferInfo.range = sizeof(ParticleInstance) * MAX_PARTICLES;
    vk::WriteDescriptorSet& culledInstancesBufferWrite { descriptorWrites[0] };
    culledInstancesBufferWrite.dstSet = _culledInstancesDescriptorSet;
    culledInstancesBufferWrite.dstBinding = 0;
    culledInstancesBufferWrite.dstArrayElement = 0;
    culledInstancesBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    culledInstancesBufferWrite.descriptorCount = 1;
    culledInstancesBufferWrite.pBufferInfo = &culledInstancesBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePass::UpdateEmittersBuffersDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    // Emitter UB (binding = 0)
    vk::DescriptorBufferInfo emitterBufferInfo {};
    emitterBufferInfo.buffer = resources->BufferResourceManager().Access(_emittersBuffer)->buffer;
    emitterBufferInfo.offset = 0;
    emitterBufferInfo.range = sizeof(Emitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[0] };
    emitterBufferWrite.dstSet = _emittersDescriptorSet;
    emitterBufferWrite.dstBinding = 0;
    emitterBufferWrite.dstArrayElement = 0;
    emitterBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    emitterBufferWrite.descriptorCount = 1;
    emitterBufferWrite.pBufferInfo = &emitterBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePass::UpdateLocalEmittersBuffersDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    // Local Emitter UB (binding = 0)
    vk::DescriptorBufferInfo localEmitterBufferInfo {};
    localEmitterBufferInfo.buffer = resources->BufferResourceManager().Access(_localEmittersBuffer)->buffer;
    localEmitterBufferInfo.offset = 0;
    localEmitterBufferInfo.range = sizeof(LocalEmitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& localEmitterBufferWrite { descriptorWrites[0] };
    localEmitterBufferWrite.dstSet = _localEmittersDescriptorSet;
    localEmitterBufferWrite.dstBinding = 0;
    localEmitterBufferWrite.dstArrayElement = 0;
    localEmitterBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    localEmitterBufferWrite.descriptorCount = 1;
    localEmitterBufferWrite.pBufferInfo = &localEmitterBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePass::UpdateDrawCommandsBufferDescriptorSet()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    // Culled Instance (binding = 0)
    vk::DescriptorBufferInfo drawCommandsBufferInfo {};
    drawCommandsBufferInfo.buffer = resources->BufferResourceManager().Access(_drawCommandsBuffer)->buffer;
    drawCommandsBufferInfo.offset = 0;
    drawCommandsBufferInfo.range = sizeof(DrawIndexedIndirectCommand);
    vk::WriteDescriptorSet& drawCommandsBufferWrite { descriptorWrites[0] };
    drawCommandsBufferWrite.dstSet = _drawCommandsDescriptorSet;
    drawCommandsBufferWrite.dstBinding = 0;
    drawCommandsBufferWrite.dstArrayElement = 0;
    drawCommandsBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    drawCommandsBufferWrite.descriptorCount = 1;
    drawCommandsBufferWrite.pBufferInfo = &drawCommandsBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePass::CreateBuffers()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    auto cmdBuffer = SingleTimeCommands(_context->VulkanContext());

    { // Draw Commands SSB
        DrawIndexedIndirectCommand indirectCommand;
        indirectCommand.command.firstIndex = 0;
        indirectCommand.command.firstInstance = 0;
        indirectCommand.command.indexCount = 6;
        indirectCommand.command.instanceCount = 0;
        indirectCommand.command.vertexOffset = 0;
        std::vector<DrawIndexedIndirectCommand> data { indirectCommand };

        BufferCreation creation {};
        creation.SetName("Particle Draw Indirect buffer")
            .SetSize(sizeof(DrawIndexedIndirectCommand))
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst);

        _drawCommandsBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(data, 0, resources->BufferResourceManager().Access(_drawCommandsBuffer)->buffer);
    }

    { // Particle SSB
        std::vector<Particle> particles(MAX_PARTICLES);
        vk::DeviceSize bufferSize = sizeof(Particle) * MAX_PARTICLES;

        BufferCreation creation {};
        creation.SetName("Particle SSB")
            .SetSize(bufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eParticle)] = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particles, 0, resources->BufferResourceManager().Access(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eParticle)])->buffer);
    }

    { // Alive and Dead SSBs
        vk::DeviceSize bufferSize = sizeof(uint32_t) * MAX_PARTICLES;

        for (size_t i = static_cast<size_t>(ParticleBufferUsage::eAliveNew); i <= static_cast<size_t>(ParticleBufferUsage::eDead); i++)
        {
            std::vector<uint32_t> indices(MAX_PARTICLES);
            if (i == static_cast<size_t>(ParticleBufferUsage::eDead))
            {
                for (uint32_t j = 0; j < MAX_PARTICLES; ++j)
                {
                    indices[j] = j;
                }
            }

            BufferCreation creation {};
            creation.SetName("Index " + std::to_string(i) + " list SSB")
                .SetSize(bufferSize)
                .SetIsMappable(false)
                .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
                .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
            _particlesBuffers[i] = resources->BufferResourceManager().Create(creation);
            cmdBuffer.CopyIntoLocalBuffer(indices, 0, resources->BufferResourceManager().Access(_particlesBuffers[i])->buffer);
        }
    }

    { // Counter SSB
        std::vector<ParticleCounters> particleCounters(1);
        vk::DeviceSize bufferSize = sizeof(ParticleCounters);

        BufferCreation creation {};
        creation.SetName("Counters SSB")
            .SetSize(bufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eCounter)] = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleCounters, 0, resources->BufferResourceManager().Access(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eCounter)])->buffer);
    }

    { // Culled Instance SSB
        vk::DeviceSize bufferSize = sizeof(ParticleInstance) * MAX_PARTICLES;
        std::vector<std::byte> culledInstances(bufferSize);

        BufferCreation creation {};
        creation.SetName("Culled Instance SSB")
            .SetSize(bufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _culledInstancesBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(culledInstances, 0, resources->BufferResourceManager().Access(_culledInstancesBuffer)->buffer);
    }

    { // Billboard vertex buffer
        std::vector<Vertex> billboardPositions = {
            Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 1.0f)), // 0
            Vertex(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 1.0f)), // 1
            Vertex(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec2(0.0f, 0.0f)), // 2
            Vertex(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec2(1.0f, 0.0f)), // 4
        };
        vk::DeviceSize bufferSize = sizeof(Vertex) * billboardPositions.size();

        BufferCreation creation {};
        creation.SetName("Billboard vertex buffer")
            .SetSize(bufferSize)
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY);
        _vertexBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(billboardPositions, 0, resources->BufferResourceManager().Access(_vertexBuffer)->buffer);
    }

    { // Billboard index buffer
        std::vector<uint32_t> billboardIndices = { 0, 1, 3, 0, 3, 2 };
        vk::DeviceSize bufferSize = sizeof(uint32_t) * billboardIndices.size();

        BufferCreation creation {};
        creation.SetName("Billboard index buffer")
            .SetSize(bufferSize)
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY);
        _indexBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(billboardIndices, 0, resources->BufferResourceManager().Access(_indexBuffer)->buffer);
    }

    { // Local Emitter UB
        std::vector<LocalEmitter> localEmitters(MAX_EMITTERS);
        vk::DeviceSize bufferSize = sizeof(LocalEmitter) * MAX_EMITTERS;

        BufferCreation creation {};
        creation.SetName("Local Emitter UB")
            .SetSize(bufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _localEmittersBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(localEmitters, 0, resources->BufferResourceManager().Access(_localEmittersBuffer)->buffer);
    }

    { // Local Emitter Staging buffer
        vk::DeviceSize bufferSize = MAX_EMITTERS * sizeof(LocalEmitter);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            util::CreateBuffer(vkContext, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, _localEmitterStagingBuffer[i], true, _localEmitterStagingBufferAllocation[i], VMA_MEMORY_USAGE_CPU_ONLY, "Local Emitter Staging buffer");
        }
    }

    cmdBuffer.Submit();

    { // Emitter UB
        vk::DeviceSize bufferSize = sizeof(Emitter) * MAX_EMITTERS;
        BufferCreation creation {};
        creation.SetName("Emitter UB")
            .SetSize(bufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _emittersBuffer = resources->BufferResourceManager().Create(creation);
    }

    { // Emitter Staging buffer
        vk::DeviceSize bufferSize = MAX_EMITTERS * sizeof(Emitter);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            util::CreateBuffer(vkContext, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, _emitterStagingBuffer[i], true, _emitterStagingBufferAllocation[i], VMA_MEMORY_USAGE_CPU_ONLY, "Emitter Staging buffer");
        }
    }
}