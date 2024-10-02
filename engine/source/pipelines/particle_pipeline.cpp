#include "pipelines/particle_pipeline.hpp"

#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"

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
    _brain.device.destroy(_pipelineLayout);
    // Buffer stuff
    for(auto& ssbo : _storageBuffers)
    {
        vmaDestroyBuffer(_brain.vmaAllocator, ssbo.buffer, ssbo.bufferAllocation);
    }
    vmaUnmapMemory(_brain.vmaAllocator, _emitterBuffer.bufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _emitterBuffer.buffer, _emitterBuffer.bufferAllocation);
    // Descriptor stuff
    _brain.device.destroy(_storageLayout);
    _brain.device.destroy(_uniformLayout);
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, float deltaTime)
{
    UpdateBuffers();

    // kick-off shader pass
    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3{ 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[0]);

    // for now try binding all
    for(auto& ssbo : _storageBuffers)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, 1,
            &ssbo.descriptorSet, 0, nullptr);
    }
    commandBuffer.dispatch(1, 1, 1);
    util::EndLabel(commandBuffer, _brain.dldi);

    // emit shader pass
    // commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 2, 1,
    //     &_emitterBuffer.descriptorSet, 0, nullptr);

    // simulate shader pass
    //commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(_pushConstants), &_pushConstants);
}

void ParticlePipeline::SpawnEmitter(glm::vec3 position, uint32_t count, EmitterType type)
{
    Emitter emitter{};
    emitter.position = position;
    emitter.count = count;
    //emitter.type = type;
    _emitters.emplace_back(emitter);
}


void ParticlePipeline::CreatePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    std::array<vk::DescriptorSetLayout, 4> layouts = { _brain.bindlessLayout, _storageLayout, _uniformLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    vk::PushConstantRange pcRange = {};
    pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
    pcRange.size = sizeof(_pushConstants);
    pcRange.offset = 0;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
                    "Failed creating particle pipeline layout!");

    _particlePaths = { "kick_off", "emit", "simulate" }; // TODO: add more shader names here later
    for (auto& shaderName : _particlePaths)
    {
        auto byteCode = shader::ReadFile("shaders/particles/" + shaderName + "-c.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.layout = _pipelineLayout;
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the geometry pipeline layout!");
        _pipelines.push_back(result.value);

        _brain.device.destroy(shaderModule);
    }
}

void ParticlePipeline::CreateDescriptorSetLayout()
{
    {   // SSBO
        std::array<vk::DescriptorSetLayoutBinding, 5> bindings{};
        for(size_t i = 0; i < 5; i++)
        {
            vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding{ bindings[i] };
            descriptorSetLayoutBinding.binding = i;
            descriptorSetLayoutBinding.descriptorCount = 1;
            descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
        }

        vk::DescriptorSetLayoutCreateInfo createInfo{};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_storageLayout),
            "Failed creating particle descriptor set layout!");
    }

    {   // UBO
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings{};

        vk::DescriptorSetLayoutBinding &descriptorSetLayoutBinding{ bindings[0] };
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo createInfo{};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_uniformLayout),
            "Failed creating particle descriptor set layout!");
    }
}

void ParticlePipeline::CreateDescriptorSets()
{
    // SSBO
    {
        vk::DescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_storageLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating SSBO descriptor sets!");

        for(size_t i = 0; i < 5; i++)
        {
            _storageBuffers[i].descriptorSet = descriptorSets[0];
        }
    }

    // Uniform
    {
        vk::DescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_uniformLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;

        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating UBO descriptor sets!");

        _emitterBuffer.descriptorSet = descriptorSets[0];
    }

    UpdateParticleDescriptorSets();
}

void ParticlePipeline::UpdateParticleDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 6> descriptorWrites {};

    // Particle SSBO (binding = 0)
    vk::DescriptorBufferInfo particleBufferInfo {};
    particleBufferInfo.buffer = _storageBuffers[static_cast<int>(SSBOUsage::PARTICLE)].buffer;
    particleBufferInfo.offset = 0;
    particleBufferInfo.range = sizeof(Particle) * MAX_PARTICLES;
    vk::WriteDescriptorSet& particleBufferWrite { descriptorWrites[0] };
    particleBufferWrite.dstSet = _storageBuffers[static_cast<int>(SSBOUsage::PARTICLE)].descriptorSet;
    particleBufferWrite.dstBinding = 0;
    particleBufferWrite.dstArrayElement = 0;
    particleBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    particleBufferWrite.descriptorCount = 1;
    particleBufferWrite.pBufferInfo = &particleBufferInfo;

    // Alive NEW list SSBO (binding = 1)
    vk::DescriptorBufferInfo aliveNEWBufferInfo {};
    aliveNEWBufferInfo.buffer = _storageBuffers[static_cast<int>(SSBOUsage::ALIVE_NEW)].buffer;
    aliveNEWBufferInfo.offset = 0;
    aliveNEWBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveNEWBufferWrite { descriptorWrites[1] };
    aliveNEWBufferWrite.dstSet = _storageBuffers[static_cast<int>(SSBOUsage::ALIVE_NEW)].descriptorSet;
    aliveNEWBufferWrite.dstBinding = 1;
    aliveNEWBufferWrite.dstArrayElement = 0;
    aliveNEWBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveNEWBufferWrite.descriptorCount = 1;
    aliveNEWBufferWrite.pBufferInfo = &aliveNEWBufferInfo;

    // Alive CURRENT list SSBO (binding = 2)
    vk::DescriptorBufferInfo aliveCURRENTBufferInfo {};
    aliveCURRENTBufferInfo.buffer = _storageBuffers[static_cast<int>(SSBOUsage::ALIVE_CURRENT)].buffer;
    aliveCURRENTBufferInfo.offset = 0;
    aliveCURRENTBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& aliveCURRENTBufferWrite { descriptorWrites[2] };
    aliveCURRENTBufferWrite.dstSet = _storageBuffers[static_cast<int>(SSBOUsage::ALIVE_CURRENT)].descriptorSet;
    aliveCURRENTBufferWrite.dstBinding = 2;
    aliveCURRENTBufferWrite.dstArrayElement = 0;
    aliveCURRENTBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    aliveCURRENTBufferWrite.descriptorCount = 1;
    aliveCURRENTBufferWrite.pBufferInfo = &aliveCURRENTBufferInfo;

    // Dead list SSBO (binding = 3)
    vk::DescriptorBufferInfo deadBufferInfo {};
    deadBufferInfo.buffer = _storageBuffers[static_cast<int>(SSBOUsage::DEAD)].buffer;
    deadBufferInfo.offset = 0;
    deadBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLES;
    vk::WriteDescriptorSet& deadBufferWrite { descriptorWrites[3] };
    deadBufferWrite.dstSet = _storageBuffers[static_cast<int>(SSBOUsage::DEAD)].descriptorSet;
    deadBufferWrite.dstBinding = 3;
    deadBufferWrite.dstArrayElement = 0;
    deadBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    deadBufferWrite.descriptorCount = 1;
    deadBufferWrite.pBufferInfo = &deadBufferInfo;

    // Counter SSBO (binding = 4)
    vk::DescriptorBufferInfo counterBufferInfo {};
    counterBufferInfo.buffer = _storageBuffers[static_cast<int>(SSBOUsage::COUNTER)].buffer;
    counterBufferInfo.offset = 0;
    counterBufferInfo.range = sizeof(uint32_t) * MAX_PARTICLE_COUNTERS;
    vk::WriteDescriptorSet& counterBufferWrite { descriptorWrites[4] };
    counterBufferWrite.dstSet = _storageBuffers[static_cast<int>(SSBOUsage::COUNTER)].descriptorSet;
    counterBufferWrite.dstBinding = 4;
    counterBufferWrite.dstArrayElement = 0;
    counterBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    counterBufferWrite.descriptorCount = 1;
    counterBufferWrite.pBufferInfo = &counterBufferInfo;

    // Emitter UBO (binding = 0)
    vk::DescriptorBufferInfo emitterBufferInfo {};
    emitterBufferInfo.buffer = _emitterBuffer.buffer;
    emitterBufferInfo.offset = 0;
    emitterBufferInfo.range = sizeof(Emitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[5] };
    emitterBufferWrite.dstSet = _emitterBuffer.descriptorSet;
    emitterBufferWrite.dstBinding = 0;
    emitterBufferWrite.dstArrayElement = 0;
    emitterBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    emitterBufferWrite.descriptorCount = 1;
    emitterBufferWrite.pBufferInfo = &emitterBufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}


void ParticlePipeline::CreateBuffers()
{
    // Allocate new command buffer for copying staging buffers
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _brain.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuffer;
    util::VK_ASSERT(vkAllocateCommandBuffers(_brain.device, &allocateInfo, &cmdBuffer), "Failed to allocate command buffer!");

    // Begin recording commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    util::VK_ASSERT(vkBeginCommandBuffer(cmdBuffer, &beginInfo), "Failed to begin recording command buffer!");

    vk::Buffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    void* data;
    {   // Particle SSBO
        std::vector<Particle> particles(MAX_PARTICLES);
        vk::DeviceSize particleBufferSize = sizeof(Particle) * MAX_PARTICLES;

        // Create and map staging buffer
        util::CreateBuffer(_brain, particleBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            stagingBuffer, true, stagingBufferAllocation,
            VMA_MEMORY_USAGE_AUTO,
            "Particle Staging buffer");
        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, stagingBufferAllocation, &data), "Failed mapping memory for staging buffer!");
        memcpy(data, particles.data(), (size_t)particleBufferSize);
        vmaUnmapMemory(_brain.vmaAllocator, stagingBufferAllocation);

        // Create and copy to SSBO
        util::CreateBuffer(_brain, particleBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            _storageBuffers[static_cast<int>(SSBOUsage::PARTICLE)].buffer, false, _storageBuffers[static_cast<int>(SSBOUsage::PARTICLE)].bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Particle SSBO");
        util::CopyBuffer(cmdBuffer, stagingBuffer, _storageBuffers[static_cast<int>(SSBOUsage::PARTICLE)].buffer, particleBufferSize);

        // Clean up
        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    {   // Alive and Dead SSBOs
        std::vector<uint32_t> indices(MAX_PARTICLES);
        vk::DeviceSize indexBufferSize = sizeof(uint32_t) * MAX_PARTICLES;

        for (size_t i = static_cast<size_t>(SSBOUsage::ALIVE_NEW); i <= static_cast<size_t>(SSBOUsage::DEAD); i++)
        {
            // Create and map staging buffer
            util::CreateBuffer(_brain, indexBufferSize,
                vk::BufferUsageFlagBits::eTransferSrc,
                stagingBuffer, true, stagingBufferAllocation,
                VMA_MEMORY_USAGE_AUTO,
                "Index Staging buffer");
            util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, stagingBufferAllocation, &data), "Failed mapping memory for staging buffer!");
            memcpy(data, indices.data(), (size_t)indexBufferSize);
            vmaUnmapMemory(_brain.vmaAllocator, stagingBufferAllocation);

            // Create and copy to SSBO
            util::CreateBuffer(_brain, indexBufferSize,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
                _storageBuffers[i].buffer, false, _storageBuffers[i].bufferAllocation,
                VMA_MEMORY_USAGE_GPU_ONLY,
                "Index list SSBO");
            util::CopyBuffer(cmdBuffer, stagingBuffer, _storageBuffers[i].buffer, indexBufferSize);

            // Clean up
            vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
        }
    }

    {   // Counter SSBO
        std::vector<uint32_t> counters(MAX_PARTICLE_COUNTERS);
        vk::DeviceSize counterBufferSize = sizeof(uint32_t) * MAX_PARTICLE_COUNTERS;

        // Create and map staging buffer
        util::CreateBuffer(_brain, counterBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            stagingBuffer, true, stagingBufferAllocation,
            VMA_MEMORY_USAGE_AUTO,
            "Counter Staging buffer");
        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, stagingBufferAllocation, &data), "Failed mapping memory for staging buffer!");
        memcpy(data, counters.data(), (size_t)counterBufferSize);
        vmaUnmapMemory(_brain.vmaAllocator, stagingBufferAllocation);

        // Create and copy to SSBO
        util::CreateBuffer(_brain, counterBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            _storageBuffers[static_cast<int>(SSBOUsage::COUNTER)].buffer, false, _storageBuffers[static_cast<int>(SSBOUsage::COUNTER)].bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Counters SSBO");
        util::CopyBuffer(cmdBuffer, stagingBuffer, _storageBuffers[static_cast<int>(SSBOUsage::COUNTER)].buffer, counterBufferSize);

        // Clean up
        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    {   // Emitter UBO
        vk::DeviceSize bufferSize = sizeof(Emitter) * MAX_EMITTERS;

        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            _emitterBuffer.buffer, true, _emitterBuffer.bufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _emitterBuffer.bufferAllocation, &_emitterBuffer.bufferMapped),
            "Failed mapping memory for UBO!");
    }
}

void ParticlePipeline::UpdateBuffers()
{
    // TODO: check if this is the way to go about swapping the alive list buffers
    std::swap(_storageBuffers[static_cast<int>(SSBOUsage::ALIVE_NEW)], _storageBuffers[static_cast<int>(SSBOUsage::ALIVE_CURRENT)]);

    memcpy(_emitterBuffer.bufferMapped, _emitters.data(), _emitters.size() * sizeof(Emitter));
    _emitters.clear();
}
