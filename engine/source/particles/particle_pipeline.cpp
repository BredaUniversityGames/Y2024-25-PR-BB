#include "particles/particle_pipeline.hpp"

#include "camera.hpp"
#include "particles/particle_util.hpp"
#include "particles/emitter_component.hpp"
#include "ECS.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"

ParticlePipeline::ParticlePipeline(const VulkanBrain& brain, const CameraResource& camera)
    : _brain(brain)
    , _camera(camera)
{
    CreateDescriptorSetLayouts();
    CreateBuffers();
    CreateDescriptorSets();
    CreatePipelines();
}

ParticlePipeline::~ParticlePipeline()
{
    // Pipeline stuff
    for (auto& pipeline : _pipelines)
    {
        _brain.device.destroy(pipeline);
    }
    for (auto& layout : _pipelineLayouts)
    {
        _brain.device.destroy(layout);
    }
    // Buffer stuff
    for (auto& storageBuffer : _particlesBuffers)
    {
        _brain.GetBufferResourceManager().Destroy(storageBuffer);
    }
    _brain.GetBufferResourceManager().Destroy(_emittersFrameData.buffer);
    for (auto& particleInstanceData : _particleInstancesFrameData)
    {
        _brain.GetBufferResourceManager().Destroy(particleInstanceData.buffer);
    }
    // Descriptor stuff
    _brain.device.destroy(_particlesBuffersDescriptorSetLayout);
    _brain.device.destroy(_emittersBufferDescriptorSetLayout);
    _brain.device.destroy(_particleInstancesDescriptorSetLayout);
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, ECS& ecs, float deltaTime)
{
    UpdateEmitters(ecs);
    UpdateBuffers();

    // Set up memory barrier to be used in between every shader stage
    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead;

    // -- kick-off shader pass --
    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<int>(ShaderStages::eKickOff)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<int>(ShaderStages::eKickOff)], 1, _particlesBuffersDescriptorSet, nullptr);

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- emit shader pass --
    util::BeginLabel(commandBuffer, "Emit particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<int>(ShaderStages::eEmit)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<int>(ShaderStages::eEmit)], 1, _particlesBuffersDescriptorSet, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<int>(ShaderStages::eEmit)], 2, _emittersFrameData.descriptorSet, nullptr);

    // spawn as many threads as there's particles to emit
    uint32_t bufferOffset = 0;
    for (bufferOffset = 0; bufferOffset < _emitters.size(); bufferOffset++)
    {
        _emitPushConstant.bufferOffset = bufferOffset;
        commandBuffer.pushConstants(_pipelineLayouts[static_cast<int>(ShaderStages::eEmit)], vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &_emitPushConstant);
        // +63 so we always dispatch at least once.
        commandBuffer.dispatch((_emitters[bufferOffset].count + 63) / 64, 1, 1);
    }

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    _emitters.clear();

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- simulate shader pass --
    util::BeginLabel(commandBuffer, "Simulate particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<int>(ShaderStages::eSimulate)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<int>(ShaderStages::eSimulate)], 1, _particlesBuffersDescriptorSet, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<int>(ShaderStages::eSimulate)], 2, _particleInstancesFrameData[currentFrame].descriptorSet, nullptr);
    // TODO: bind camera resource for set 3

    _simulatePushConstant.deltaTime = deltaTime;
    commandBuffer.pushConstants(_pipelineLayouts[static_cast<int>(ShaderStages::eSimulate)], vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &_simulatePushConstant);

    commandBuffer.dispatch(MAX_PARTICLES / 256, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllGraphics, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- instanced rendering --
}

void ParticlePipeline::UpdateEmitters(ECS& ecs)
{
    auto view = ecs._registry.view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& component = view.get<EmitterComponent>(entity);
        if (component.timesToEmit != 0)
        {
            // TODO: do something with particle type later
            _emitters.emplace_back(component.emitter);
            spdlog::info("Emitter received!");
            component.timesToEmit--;
        }
    }
}

void ParticlePipeline::UpdateBuffers()
{
    // TODO: check if this swapping works
    std::swap(_particlesBuffers[static_cast<int>(SSBUsage::eAliveNew)], _particlesBuffers[static_cast<int>(SSBUsage::eAliveCurrent)]);

    memcpy(_brain.GetBufferResourceManager().Access(_emittersFrameData.buffer)->mappedPtr, _emitters.data(), _emitters.size() * sizeof(Emitter));
}

void ParticlePipeline::CreatePipelines()
{
    { // kick-off
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 2> layouts = { _brain.bindlessLayout, _particlesBuffersDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<int>(ShaderStages::eKickOff)]),
            "Failed creating kick_off pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/kick_off.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<int>(ShaderStages::eKickOff)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the kick_off compute pipeline!");
        _pipelines[static_cast<int>(ShaderStages::eKickOff)] = result.value;

        _brain.device.destroy(shaderModule);
    }

    { // emit
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 3> layouts = { _brain.bindlessLayout, _particlesBuffersDescriptorSetLayout, _emittersBufferDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

        vk::PushConstantRange pcRange = {};
        pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pcRange.offset = 0;
        pcRange.size = sizeof(_emitPushConstant);

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<int>(ShaderStages::eEmit)]),
            "Failed creating emit pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/emit.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<int>(ShaderStages::eEmit)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the emit compute pipeline!");
        _pipelines[static_cast<int>(ShaderStages::eEmit)] = result.value;

        _brain.device.destroy(shaderModule);
    }

    { // simulate
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 4> layouts = { _brain.bindlessLayout, _particlesBuffersDescriptorSetLayout, _particleInstancesDescriptorSetLayout, _camera.DescriptorSetLayout() };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

        vk::PushConstantRange pcRange = {};
        pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pcRange.offset = 0;
        pcRange.size = sizeof(_simulatePushConstant);

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<int>(ShaderStages::eSimulate)]),
            "Failed creating simulate pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/simulate.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<int>(ShaderStages::eSimulate)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the simulate compute pipeline!");
        _pipelines[static_cast<int>(ShaderStages::eSimulate)] = result.value;

        _brain.device.destroy(shaderModule);
    }

    { // instanced rendering
        // TODO: set-up and implement graphics pipeline
    }
}

void ParticlePipeline::CreateDescriptorSetLayouts()
{
    { // Particle Storage Buffers
        std::array<vk::DescriptorSetLayoutBinding, 5> bindings {};
        for (size_t i = 0; i < 5; i++)
        {
            vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[i] };
            descriptorSetLayoutBinding.binding = i;
            descriptorSetLayoutBinding.descriptorCount = 1;
            descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
        }

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_particlesBuffersDescriptorSetLayout),
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

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_emittersBufferDescriptorSetLayout),
            "Failed creating emitter buffer descriptor set layout!");
    }

    { // Particle Instances Storage Buffer
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

        vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_particleInstancesDescriptorSetLayout),
            "Failed creating particle instances buffer descriptor set layout!");
    }
}

void ParticlePipeline::CreateDescriptorSets()
{
    { // Particle Storage Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_particlesBuffersDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Storage Buffer descriptor sets!");

        _particlesBuffersDescriptorSet = descriptorSets[0];
    }

    { // Emitter Uniform Buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_emittersBufferDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Emitter Uniform Buffer descriptor sets!");

        _emittersFrameData.descriptorSet = descriptorSets[0];
    }

    UpdateParticleBuffersDescriptorSets();

    { // Particle Instances Storage Buffer
        std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
        std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
            { l = _particleInstancesDescriptorSetLayout; });
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocateInfo.pSetLayouts = layouts.data();

        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Instances Storage Buffer descriptor sets!");

        for(size_t i = 0; i < descriptorSets.size(); i++)
        {
            _particleInstancesFrameData[i].descriptorSet = descriptorSets[i];
            UpdateParticleInstancesData(i);
        }
    }
}

void ParticlePipeline::UpdateParticleBuffersDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 6> descriptorWrites {};

    // Particle SSB (binding = 0)
    int index = static_cast<int>(SSBUsage::eParticle);
    vk::DescriptorBufferInfo particleBufferInfo {};
    particleBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particlesBuffers[index])->buffer;
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
    index = static_cast<int>(SSBUsage::eAliveNew);
    vk::DescriptorBufferInfo aliveNEWBufferInfo {};
    aliveNEWBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particlesBuffers[index])->buffer;
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
    index = static_cast<int>(SSBUsage::eAliveCurrent);
    vk::DescriptorBufferInfo aliveCURRENTBufferInfo {};
    aliveCURRENTBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particlesBuffers[index])->buffer;
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
    index = static_cast<int>(SSBUsage::eDead);
    vk::DescriptorBufferInfo deadBufferInfo {};
    deadBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particlesBuffers[index])->buffer;
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
    index = static_cast<int>(SSBUsage::eCounter);
    vk::DescriptorBufferInfo counterBufferInfo {};
    counterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particlesBuffers[index])->buffer;
    counterBufferInfo.offset = 0;
    counterBufferInfo.range = sizeof(ParticleCounters);
    vk::WriteDescriptorSet& counterBufferWrite { descriptorWrites[index] };
    counterBufferWrite.dstSet = _particlesBuffersDescriptorSet;
    counterBufferWrite.dstBinding = index;
    counterBufferWrite.dstArrayElement = 0;
    counterBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    counterBufferWrite.descriptorCount = 1;
    counterBufferWrite.pBufferInfo = &counterBufferInfo;

    // Emitter UB (binding = 0)
    vk::DescriptorBufferInfo emitterBufferInfo {};
    emitterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_emittersFrameData.buffer)->buffer;
    emitterBufferInfo.offset = 0;
    emitterBufferInfo.range = sizeof(Emitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[5] };
    emitterBufferWrite.dstSet = _emittersFrameData.descriptorSet;
    emitterBufferWrite.dstBinding = 0;
    emitterBufferWrite.dstArrayElement = 0;
    emitterBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    emitterBufferWrite.descriptorCount = 1;
    emitterBufferWrite.pBufferInfo = &emitterBufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePipeline::UpdateParticleInstancesData(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo particleInstancesBufferInfo {};
    particleInstancesBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particleInstancesFrameData[frameIndex].buffer)->buffer;
    particleInstancesBufferInfo.offset = 0;
    particleInstancesBufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[0] };
    emitterBufferWrite.dstSet = _particleInstancesFrameData[frameIndex].descriptorSet;
    emitterBufferWrite.dstBinding = 0;
    emitterBufferWrite.dstArrayElement = 0;
    emitterBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    emitterBufferWrite.descriptorCount = 1;
    emitterBufferWrite.pBufferInfo = &particleInstancesBufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}


void ParticlePipeline::CreateBuffers()
{
    auto cmdBuffer = SingleTimeCommands(_brain);

    { // Particle SSB
        std::vector<Particle> particles(MAX_PARTICLES);
        vk::DeviceSize particleBufferSize = sizeof(Particle) * MAX_PARTICLES;

        BufferCreation creation {};
        creation.SetName("Particle SSB")
            .SetSize(particleBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _particlesBuffers[static_cast<int>(SSBUsage::eParticle)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particles, 0, _brain.GetBufferResourceManager().Access(_particlesBuffers[static_cast<int>(SSBUsage::eParticle)])->buffer);
    }

    { // Alive and Dead SSBs
        vk::DeviceSize indexBufferSize = sizeof(uint32_t) * MAX_PARTICLES;

        for (size_t i = static_cast<size_t>(SSBUsage::eAliveNew); i <= static_cast<size_t>(SSBUsage::eDead); i++)
        {
            std::vector<uint32_t> indices(MAX_PARTICLES);
            if (i == static_cast<size_t>(SSBUsage::eDead))
            {
                for (uint32_t j = 0; j < MAX_PARTICLES; ++j)
                {
                    indices[j] = j;
                }
            }

            BufferCreation creation {};
            creation.SetName("Index list SSB")
                .SetSize(indexBufferSize)
                .SetIsMappable(false)
                .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
                .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
            _particlesBuffers[i] = _brain.GetBufferResourceManager().Create(creation);
            cmdBuffer.CopyIntoLocalBuffer(indices, 0, _brain.GetBufferResourceManager().Access(_particlesBuffers[i])->buffer);
        }
    }

    { // Counter SSB
        std::vector<ParticleCounters> particleCounters(1);
        vk::DeviceSize counterBufferSize = sizeof(ParticleCounters);

        BufferCreation creation {};
        creation.SetName("Counters SSB")
            .SetSize(counterBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _particlesBuffers[static_cast<int>(SSBUsage::eCounter)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleCounters, 0, _brain.GetBufferResourceManager().Access(_particlesBuffers[static_cast<int>(SSBUsage::eCounter)])->buffer);
    }

    { // Particle Instances SSB
        for(size_t i = 0; i < _particleInstancesFrameData.size(); i++)
        {
            std::vector<ParticleInstance> particleInstances(MAX_PARTICLES);
            vk::DeviceSize particleInstancesBufferSize = sizeof(ParticleInstance) * MAX_PARTICLES;

            BufferCreation creation {};
            creation.SetName("Particle Instances SSB")
                .SetSize(particleInstancesBufferSize)
                .SetIsMappable(false)
                .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
                .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
            _particleInstancesFrameData[i].buffer = _brain.GetBufferResourceManager().Create(creation);
            cmdBuffer.CopyIntoLocalBuffer(particleInstances, 0, _brain.GetBufferResourceManager().Access(_particleInstancesFrameData[i].buffer)->buffer);
        }
    }

    cmdBuffer.Submit();

    { // Emitter UB
        vk::DeviceSize bufferSize = sizeof(Emitter) * MAX_EMITTERS;
        BufferCreation creation {};
        creation.SetName("Emitter UB")
            .SetSize(bufferSize)
            .SetIsMappable(true)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer);
        _emittersFrameData.buffer = _brain.GetBufferResourceManager().Create(creation);
    }
}
