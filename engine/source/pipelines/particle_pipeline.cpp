#include "pipelines/particle_pipeline.hpp"

#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"

ParticlePipeline::ParticlePipeline(const VulkanBrain& brain, const CameraStructure& camera)
    : _brain(brain)
    , _camera(camera)
{
    CreateDescriptorSetLayout();
    CreateDescriptorSets();
    CreateBuffers();
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
    vmaDestroyBuffer(_brain.vmaAllocator, _particleBuffer.buffer, _particleBuffer.bufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _deadList.buffer, _deadList.bufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _counterBuffer.buffer, _counterBuffer.bufferAllocation);
    for(size_t i = 0; i < 2; i++)
    {
        vmaDestroyBuffer(_brain.vmaAllocator, _aliveList[i].buffer, _aliveList[i].bufferAllocation);
    }
    // Descriptor stuff
    _brain.device.destroy(_descriptorSetLayout);
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, float deltaTime)
{
    UpdateBuffers();

    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3{ 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[0]);

    commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(_pushConstants), &_pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, 1,
                                    &_descriptorSet, 0, nullptr);

    commandBuffer.dispatch(256, 1, 1);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void ParticlePipeline::CreatePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    std::array<vk::DescriptorSetLayout, 3> layouts = { _brain.bindlessLayout, _descriptorSetLayout, _camera.descriptorSetLayout };
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

    _particlePaths = { "kick_off" }; // TODO: add more shader names here
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

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_descriptorSetLayout),
        "Failed creating particle descriptor set layout!");
}

void ParticlePipeline::CreateDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_descriptorSetLayout;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, &_descriptorSet),
                    "Failed allocating particle descriptor sets!");
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
            _particleBuffer.buffer, false, _particleBuffer.bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Particle SSBO");
        util::CopyBuffer(cmdBuffer, stagingBuffer, _particleBuffer.buffer, particleBufferSize);

        // Clean up
        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    {   // Index SSBOs
        // Dead SSBO
        std::vector<uint32_t> indices(MAX_PARTICLES);
        vk::DeviceSize indexBufferSize = sizeof(uint32_t) * MAX_PARTICLES;

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
            _deadList.buffer, false, _deadList.bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Dead list SSBO");
        util::CopyBuffer(cmdBuffer, stagingBuffer, _deadList.buffer, indexBufferSize);

        // Clean up
        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);

        // Alive SSBOs
        for (size_t i = 0; i < 2; i++)
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
                _aliveList[i].buffer, false, _aliveList[i].bufferAllocation,
                VMA_MEMORY_USAGE_GPU_ONLY,
                "Alive list SSBO");
            util::CopyBuffer(cmdBuffer, stagingBuffer, _aliveList[i].buffer, indexBufferSize);

            // Clean up
            vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
        }
    }

    {   // Counter SSBO
        // TODO: remove hardcoded values
        std::vector<uint32_t> counters(3);
        vk::DeviceSize counterBufferSize = sizeof(uint32_t) * 3;

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
            _counterBuffer.buffer, false, _counterBuffer.bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Counters SSBO");
        util::CopyBuffer(cmdBuffer, stagingBuffer, _counterBuffer.buffer, counterBufferSize);

        // Clean up
        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }
}

void ParticlePipeline::UpdateBuffers()
{
    // TODO: check if this is the way to do this lol and maybe we can do async at some point..
    std::swap(_aliveList[0], _aliveList[1]);
}
