#include "particles/particle_pipeline.hpp"

#include "camera.hpp"
#include "particles/particle_util.hpp"
#include "particles/emitter_component.hpp"
#include "ECS.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"

ParticlePipeline::ParticlePipeline(const VulkanBrain& brain, const CameraStructure& camera)
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
    for(auto& layout : _pipelineLayouts)
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

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, ECS& ecs)
{
    UpdateEmitters(ecs);
    UpdateBuffers();

    // Set up memory barrier to be used in between every shader stage
    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead;

    // -- kick-off shader pass --
    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[0]);

    // bind storage buffers
    for (uint32_t i = 0; i < _storageBufferDescriptorSets.size(); i++)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[0], 1, 1,
            &_storageBufferDescriptorSets[i], 0, nullptr);
    }

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 },
        1, &memoryBarrier, 0, nullptr, 0, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- emit shader pass --
    util::BeginLabel(commandBuffer, "Emit particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[1]);

    // bind buffers
    for (uint32_t i = 0; i < _storageBufferDescriptorSets.size(); i++)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[1], 1, 1,
            &_storageBufferDescriptorSets[i], 0, nullptr);
    }
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[1], 2, 1,
        &_emitterBufferDescriptorSet, 0, nullptr);

    // spawn as many threads as there's particles to emit
    uint32_t bufferOffset = 0;
    for (bufferOffset = 0; bufferOffset < _emitters.size(); bufferOffset++)
    {
        _emitPushConstant.bufferOffset = bufferOffset;
        commandBuffer.pushConstants(_pipelineLayouts[1], vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &_emitPushConstant);
        // +63 so we always dispatch at least once.
        commandBuffer.dispatch((_emitters[bufferOffset].count + 63) / 64, 1, 1);
    }

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 },
        1, &memoryBarrier, 0, nullptr, 0, nullptr);

    _emitters.clear();

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- simulate shader pass --
    // _simulatePushConstant.deltaTime = deltaTime;
    // commandBuffer.pushConstants(_pipelineLayouts[2], vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &_simulatePushConstant);
    // commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{ 0 },
    // 1, &memoryBarrier, 0, nullptr, 0, nullptr);

    // -- finish shader pass --
    util::BeginLabel(commandBuffer, "Finish particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[3]);

    // bind storage buffers
    for (uint32_t i = 0; i < _storageBufferDescriptorSets.size(); i++)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[3], 1, 1,
            &_storageBufferDescriptorSets[i], 0, nullptr);
    }

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 },
        1, &memoryBarrier, 0, nullptr, 0, nullptr);

    util::EndLabel(commandBuffer, _brain.dldi);

    // TODO: execution barrier between compute and graphics render

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
    std::array<vk::DescriptorSetLayout, 4> layouts = { _brain.bindlessLayout, _storageLayout, _uniformLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    _shaderPaths = { "kick_off", "emit", "simulate", "finish" };
    for (uint32_t i = 0; i < _shaderPaths.size(); i++)
    {
        vk::PushConstantRange pcRange = {};
        pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pcRange.offset = 0;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

        if(_shaderPaths[i] == "emit")
        {
            pcRange.size = sizeof(_emitPushConstant);
            pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
        }
        else if(_shaderPaths[i] == "simulate")
        {
            pcRange.size = sizeof(_simulatePushConstant);
            pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;
        }
        else
        {
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        }

        _pipelineLayouts.push_back(vk::PipelineLayout{});
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
    {   // Shader Storage Buffer
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

    {   // Uniform Buffer
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
    {   // Shader Storage Buffer
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_storageLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Shader Storage Buffer descriptor sets!");

        for (size_t i = 0; i < 5; i++)
        {
            _storageBufferDescriptorSets[i] = descriptorSets[0];
        }
    }

    {   // Uniform Buffer
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
    vk::DescriptorBufferInfo particleBufferInfo {};
    particleBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eParticle)])->buffer;
    particleBufferInfo.offset = 0;
    particleBufferInfo.range = sizeof(Particle) * MAX_PARTICLES;
    vk::WriteDescriptorSet& particleBufferWrite { descriptorWrites[0] };
    particleBufferWrite.dstSet = _storageBufferDescriptorSets[static_cast<int>(SSBUsage::eParticle)];
    particleBufferWrite.dstBinding = 0;
    particleBufferWrite.dstArrayElement = 0;
    particleBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    particleBufferWrite.descriptorCount = 1;
    particleBufferWrite.pBufferInfo = &particleBufferInfo;

    // Alive NEW list SSB (binding = 1)
    vk::DescriptorBufferInfo aliveNEWBufferInfo {};
    aliveNEWBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eAliveNew)])->buffer;
    aliveNEWBufferInfo.offset = 0;
    aliveNEWBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveNEWBufferWrite { descriptorWrites[1] };
    aliveNEWBufferWrite.dstSet = _storageBufferDescriptorSets[static_cast<int>(SSBUsage::eAliveNew)];
    aliveNEWBufferWrite.dstBinding = 1;
    aliveNEWBufferWrite.dstArrayElement = 0;
    aliveNEWBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveNEWBufferWrite.descriptorCount = 1;
    aliveNEWBufferWrite.pBufferInfo = &aliveNEWBufferInfo;

    // Alive CURRENT list SSB (binding = 2)
    vk::DescriptorBufferInfo aliveCURRENTBufferInfo {};
    aliveCURRENTBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eAliveCurrent)])->buffer;
    aliveCURRENTBufferInfo.offset = 0;
    aliveCURRENTBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveCURRENTBufferWrite { descriptorWrites[2] };
    aliveCURRENTBufferWrite.dstSet = _storageBufferDescriptorSets[static_cast<int>(SSBUsage::eAliveCurrent)];
    aliveCURRENTBufferWrite.dstBinding = 2;
    aliveCURRENTBufferWrite.dstArrayElement = 0;
    aliveCURRENTBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveCURRENTBufferWrite.descriptorCount = 1;
    aliveCURRENTBufferWrite.pBufferInfo = &aliveCURRENTBufferInfo;

    // Dead list SSB (binding = 3)
    vk::DescriptorBufferInfo deadBufferInfo {};
    deadBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eDead)])->buffer;
    deadBufferInfo.offset = 0;
    deadBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& deadBufferWrite { descriptorWrites[3] };
    deadBufferWrite.dstSet = _storageBufferDescriptorSets[static_cast<int>(SSBUsage::eDead)];
    deadBufferWrite.dstBinding = 3;
    deadBufferWrite.dstArrayElement = 0;
    deadBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    deadBufferWrite.descriptorCount = 1;
    deadBufferWrite.pBufferInfo = &deadBufferInfo;

    // Counter SSB (binding = 4)
    vk::DescriptorBufferInfo counterBufferInfo {};
    counterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eCounter)])->buffer;
    counterBufferInfo.offset = 0;
    counterBufferInfo.range = sizeof(ParticleCounters);
    vk::WriteDescriptorSet& counterBufferWrite { descriptorWrites[4] };
    counterBufferWrite.dstSet = _storageBufferDescriptorSets[static_cast<int>(SSBUsage::eCounter)];
    counterBufferWrite.dstBinding = 4;
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

    {   // Particle SSB
        std::vector<Particle> particles(MAX_PARTICLES);
        vk::DeviceSize particleBufferSize = sizeof(Particle) * MAX_PARTICLES;

        // Create and copy to SSB
        BufferCreation creation {};
        creation.SetName("Particle SSB")
            .SetSize(particleBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _storageBuffers[static_cast<int>(SSBUsage::eParticle)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particles, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eParticle)])->buffer);
    }

    {   // Alive and Dead SSBs
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

            // Create and copy to SSB
            BufferCreation creation {};
            creation.SetName("Index list SSB")
                .SetSize(indexBufferSize)
                .SetIsMappable(false)
                .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
            _storageBuffers[i] = _brain.GetBufferResourceManager().Create(creation);
            cmdBuffer.CopyIntoLocalBuffer(indices, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[i])->buffer);
        }
    }

    {   // Counter SSB
        std::vector<ParticleCounters> particleCounters(1);
        particleCounters[0].deadCount = MAX_PARTICLES;
        vk::DeviceSize counterBufferSize = sizeof(ParticleCounters);

        // Create and copy to SSB
        BufferCreation creation {};
        creation.SetName("Counters SSB")
            .SetSize(counterBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _storageBuffers[static_cast<int>(SSBUsage::eCounter)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleCounters, 0, _brain.GetBufferResourceManager().Access(_storageBuffers[static_cast<int>(SSBUsage::eCounter)])->buffer);
    }

    cmdBuffer.Submit();

    {   // Emitter UB
        vk::DeviceSize bufferSize = sizeof(Emitter) * MAX_EMITTERS;
        BufferCreation creation {};
        creation.SetName("Emitter UB")
            .SetSize(bufferSize)
            .SetIsMappable(true)
            .SetMemoryUsage(VMA_MEMORY_USAGE_CPU_ONLY)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer);
        _emitterBuffer = _brain.GetBufferResourceManager().Create(creation);
    }
}
