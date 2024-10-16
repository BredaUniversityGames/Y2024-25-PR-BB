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
    CreateDescriptorSetLayout();
    CreateBuffers();
    CreateDescriptorSets();
    CreatePipeline();
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
    for (auto& storageBuffer : _storageBuffers)
    {
        _brain.GetBufferResourceManager().Destroy(storageBuffer);
    }
    _brain.GetBufferResourceManager().Destroy(_emitterBuffer);
    // Descriptor stuff
    _brain.device.destroy(_storageLayout);
    _brain.device.destroy(_uniformLayout);
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, ECS& ecs, float deltaTime)
{
   // UpdateEmitters(ecs);
    UpdateBuffers();

    // Set up memory barrier to be used in between every shader stage
    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead;

    // -- kick-off shader pass --
    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[0]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[0], 1, _storageBufferDescriptorSet, nullptr);

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- emit shader pass --
    util::BeginLabel(commandBuffer, "Emit particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[1]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[1], 1, _storageBufferDescriptorSet, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[1], 2, _emitterBufferDescriptorSet, nullptr);

    // spawn as many threads as there's particles to emit
    uint32_t bufferOffset = 0;
    for (bufferOffset = 0; bufferOffset < _emitters.size(); bufferOffset++)
    {
        _emitPushConstant.bufferOffset = bufferOffset;
        commandBuffer.pushConstants(_pipelineLayouts[1], vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &_emitPushConstant);
        // +63 so we always dispatch at least once.
        commandBuffer.dispatch((_emitters[bufferOffset].count + 63) / 64, 1, 1);
    }

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    _emitters.clear();

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- simulate shader pass --
    util::BeginLabel(commandBuffer, "Simulate particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[2]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[2], 1, _storageBufferDescriptorSet, nullptr);

    _simulatePushConstant.deltaTime = deltaTime;
    commandBuffer.pushConstants(_pipelineLayouts[2], vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &_simulatePushConstant);

    commandBuffer.dispatch(MAX_PARTICLES / 256, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- finish shader pass --
    util::BeginLabel(commandBuffer, "Finish particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[3]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[3], 1, _storageBufferDescriptorSet, nullptr);

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, vk::DependencyFlags { 0 }, memoryBarrier, nullptr, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);


    // -- indirect draw call rendering --
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
    std::swap(_storageBuffers[static_cast<int>(SSBUsage::eAliveNew)], _storageBuffers[static_cast<int>(SSBUsage::eAliveCurrent)]);

    memcpy(_brain.GetBufferResourceManager().Access(_emitterBuffer)->mappedPtr, _emitters.data(), _emitters.size() * sizeof(Emitter));
}

void ParticlePipeline::CreatePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 4> layouts = { _brain.bindlessLayout, _storageLayout, _uniformLayout, _camera.DescriptorSetLayout() };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    _shaderPaths = { "kick_off", "emit", "simulate", "finish" };
    for (uint32_t i = 0; i < _shaderPaths.size(); i++)
    {
        vk::PushConstantRange pcRange = {};
        pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pcRange.offset = 0;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

        if (_shaderPaths[i] == "emit")
        {
            pcRange.size = sizeof(_emitPushConstant);
            pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
        }
        else if (_shaderPaths[i] == "simulate")
        {
            pcRange.size = sizeof(_simulatePushConstant);
            pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
        }
        else
        {
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        }

        _pipelineLayouts.push_back(vk::PipelineLayout {});
        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[i]),
            "Failed creating " + _shaderPaths[i] + " pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/" + _shaderPaths[i] + ".comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[i];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the " + _shaderPaths[i] + " compute pipeline!");
        _pipelines.push_back(result.value);

        _brain.device.destroy(shaderModule);
    }
}

void ParticlePipeline::CreateDescriptorSetLayout()
{
    { // Shader Storage Buffer
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

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_storageLayout),
            "Failed creating particle descriptor set layout!");
    }

    { // Uniform Buffer
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

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_uniformLayout),
            "Failed creating particle descriptor set layout!");
    }
}

void ParticlePipeline::CreateDescriptorSets()
{
    { // Shader Storage Buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_storageLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Shader Storage Buffer descriptor sets!");

        _storageBufferDescriptorSet = descriptorSets[0];
    }

    { // Uniform Buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_uniformLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Uniform Buffer descriptor sets!");

        _emitterBufferDescriptorSet = descriptorSets[0];
    }

    UpdateParticleDescriptorSets();
}

void ParticlePipeline::UpdateParticleDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 6> descriptorWrites {};

    // Particle SSB (binding = 0)
    int index = static_cast<int>(SSBUsage::eParticle);
    vk::DescriptorBufferInfo particleBufferInfo {};
    particleBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[index])->buffer;
    particleBufferInfo.offset = 0;
    particleBufferInfo.range = sizeof(Particle) * MAX_PARTICLES;
    vk::WriteDescriptorSet& particleBufferWrite { descriptorWrites[index] };
    particleBufferWrite.dstSet = _storageBufferDescriptorSet;
    particleBufferWrite.dstBinding = index;
    particleBufferWrite.dstArrayElement = 0;
    particleBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    particleBufferWrite.descriptorCount = 1;
    particleBufferWrite.pBufferInfo = &particleBufferInfo;

    // Alive NEW list SSB (binding = 1)
    index = static_cast<int>(SSBUsage::eAliveNew);
    vk::DescriptorBufferInfo aliveNEWBufferInfo {};
    aliveNEWBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[index])->buffer;
    aliveNEWBufferInfo.offset = 0;
    aliveNEWBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveNEWBufferWrite { descriptorWrites[index] };
    aliveNEWBufferWrite.dstSet = _storageBufferDescriptorSet;
    aliveNEWBufferWrite.dstBinding = index;
    aliveNEWBufferWrite.dstArrayElement = 0;
    aliveNEWBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveNEWBufferWrite.descriptorCount = 1;
    aliveNEWBufferWrite.pBufferInfo = &aliveNEWBufferInfo;

    // Alive CURRENT list SSB (binding = 2)
    index = static_cast<int>(SSBUsage::eAliveCurrent);
    vk::DescriptorBufferInfo aliveCURRENTBufferInfo {};
    aliveCURRENTBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[index])->buffer;
    aliveCURRENTBufferInfo.offset = 0;
    aliveCURRENTBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveCURRENTBufferWrite { descriptorWrites[index] };
    aliveCURRENTBufferWrite.dstSet = _storageBufferDescriptorSet;
    aliveCURRENTBufferWrite.dstBinding = index;
    aliveCURRENTBufferWrite.dstArrayElement = 0;
    aliveCURRENTBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveCURRENTBufferWrite.descriptorCount = 1;
    aliveCURRENTBufferWrite.pBufferInfo = &aliveCURRENTBufferInfo;

    // Dead list SSB (binding = 3)
    index = static_cast<int>(SSBUsage::eDead);
    vk::DescriptorBufferInfo deadBufferInfo {};
    deadBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[index])->buffer;
    deadBufferInfo.offset = 0;
    deadBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& deadBufferWrite { descriptorWrites[index] };
    deadBufferWrite.dstSet = _storageBufferDescriptorSet;
    deadBufferWrite.dstBinding = index;
    deadBufferWrite.dstArrayElement = 0;
    deadBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    deadBufferWrite.descriptorCount = 1;
    deadBufferWrite.pBufferInfo = &deadBufferInfo;

    // Counter SSB (binding = 4)
    index = static_cast<int>(SSBUsage::eCounter);
    vk::DescriptorBufferInfo counterBufferInfo {};
    counterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[index])->buffer;
    counterBufferInfo.offset = 0;
    counterBufferInfo.range = sizeof(ParticleCounters);
    vk::WriteDescriptorSet& counterBufferWrite { descriptorWrites[index] };
    counterBufferWrite.dstSet = _storageBufferDescriptorSet;
    counterBufferWrite.dstBinding = index;
    counterBufferWrite.dstArrayElement = 0;
    counterBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    counterBufferWrite.descriptorCount = 1;
    counterBufferWrite.pBufferInfo = &counterBufferInfo;

    // Emitter UB (binding = 0)
    vk::DescriptorBufferInfo emitterBufferInfo {};
    emitterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_emitterBuffer)->buffer;
    emitterBufferInfo.offset = 0;
    emitterBufferInfo.range = sizeof(Emitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[5] };
    emitterBufferWrite.dstSet = _emitterBufferDescriptorSet;
    emitterBufferWrite.dstBinding = 0;
    emitterBufferWrite.dstArrayElement = 0;
    emitterBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    emitterBufferWrite.descriptorCount = 1;
    emitterBufferWrite.pBufferInfo = &emitterBufferInfo;

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
        _storageBuffers[static_cast<int>(SSBUsage::eParticle)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particles, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eParticle)])->buffer);
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
            _storageBuffers[i] = _brain.GetBufferResourceManager().Create(creation);
            cmdBuffer.CopyIntoLocalBuffer(indices, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[i])->buffer);
        }
    }

    { // Counter SSB
        std::vector<ParticleCounters> particleCounters(1);
        particleCounters[0].deadCount = MAX_PARTICLES;
        vk::DeviceSize counterBufferSize = sizeof(ParticleCounters);

        BufferCreation creation {};
        creation.SetName("Counters SSB")
            .SetSize(counterBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _storageBuffers[static_cast<int>(SSBUsage::eCounter)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleCounters, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eCounter)])->buffer);
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
        _emitterBuffer = _brain.GetBufferResourceManager().Create(creation);
    }
}
