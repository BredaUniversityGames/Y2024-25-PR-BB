#include "particles/particle_pipeline.hpp"

#include "camera.hpp"
#include "swap_chain.hpp"
#include "particles/particle_util.hpp"
#include "particles/emitter_component.hpp"
#include "ECS.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"

ParticlePipeline::ParticlePipeline(const VulkanBrain& brain, const CameraResource& camera, const SwapChain& swapChain)
    : _brain(brain)
    , _camera(camera)
    , _swapChain(swapChain)
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
    _brain.GetBufferResourceManager().Destroy(_particleInstancesBuffer);
    _brain.GetBufferResourceManager().Destroy(_culledIndicesBuffer);
    _brain.GetBufferResourceManager().Destroy(_emittersBuffer);
    vmaDestroyBuffer(_brain.vmaAllocator, _stagingBuffer, _stagingBufferAllocation);
    // Descriptor stuff
    _brain.device.destroy(_particlesBuffersDescriptorSetLayout);
    _brain.device.destroy(_emittersBufferDescriptorSetLayout);
    _brain.device.destroy(_instancesDescriptorSetLayout);
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, ECS& ecs, float deltaTime)
{
    vk::MemoryBarrier memoryBarrier {};
    memoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    memoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead;

    UpdateEmitters(ecs);
    UpdateBuffers(commandBuffer);

    // -- kick-off shader pass --
    util::BeginLabel(commandBuffer, "Kick-off particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eKickOff)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)], 2, _instancesDescriptorSet, {});

    commandBuffer.dispatch(1, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- emit shader pass --
    util::BeginLabel(commandBuffer, "Emit particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eEmit)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], 2, _emittersDescriptorSet, {});

    // spawn as many threads as there's particles to emit
    uint32_t bufferOffset = 0;
    for (bufferOffset = 0; bufferOffset < _emitters.size(); bufferOffset++)
    {
        _emitPushConstant.bufferOffset = bufferOffset;
        commandBuffer.pushConstants(_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)], vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &_emitPushConstant);
        // +63 so we always dispatch at least once.
        commandBuffer.dispatch((_emitters[bufferOffset].count + 63) / 64, 1, 1);
    }

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    _emitters.clear();

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- simulate shader pass --
    util::BeginLabel(commandBuffer, "Simulate particle pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipelines[static_cast<uint32_t>(ShaderStages::eSimulate)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 1, _particlesBuffersDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], 2, _instancesDescriptorSet, {});

    _simulatePushConstant.deltaTime = deltaTime;
    commandBuffer.pushConstants(_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)], vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &_simulatePushConstant);

    commandBuffer.dispatch(MAX_PARTICLES / 256, 1, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllGraphics, vk::DependencyFlags { 0 }, memoryBarrier, {}, {});

    util::EndLabel(commandBuffer, _brain.dldi);

    // -- instanced rendering --
    util::BeginLabel(commandBuffer, "Particle rendering pass", glm::vec3 { 255.0f, 105.0f, 180.0f } / 255.0f, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines[static_cast<uint32_t>(ShaderStages::eRenderInstanced)]);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderInstanced)], 0, _brain.bindlessSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderInstanced)], 1, _instancesDescriptorSet, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderInstanced)], 2, _camera.DescriptorSet(currentFrame), {});

    // TODO: read particle instance count from culledBuffer and drawIndexed

    util::EndLabel(commandBuffer, _brain.dldi);
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
            component.timesToEmit--; // TODO: possibly move the updating of emitters to the GPU
        }
    }
}

void ParticlePipeline::UpdateBuffers(vk::CommandBuffer commandBuffer)
{
    // TODO: check if this swapping works
    std::swap(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eAliveNew)], _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eAliveCurrent)]);

    if (_emitters.size() > 0)
    {
        vk::DeviceSize bufferSize = _emitters.size() * sizeof(Emitter);

        vmaCopyMemoryToAllocation(_brain.vmaAllocator, _emitters.data(), _stagingBufferAllocation, 0, bufferSize);
        util::CopyBuffer(commandBuffer, _stagingBuffer, _brain.GetBufferResourceManager().Access(_emittersBuffer)->buffer, bufferSize);

        vk::BufferMemoryBarrier barrier{};
        barrier.buffer = _brain.GetBufferResourceManager().Access(_emittersBuffer)->buffer;
        barrier.size = bufferSize;
        barrier.offset = 0;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{ 0 }, {}, barrier, {});
    }
}

void ParticlePipeline::CreatePipelines()
{
    { // kick-off
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 3> layouts = { _brain.bindlessLayout, _particlesBuffersDescriptorSetLayout, _instancesDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)]),
            "Failed creating kick_off pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/kick_off.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eKickOff)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the kick_off compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eKickOff)] = result.value;

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

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)]),
            "Failed creating emit pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/emit.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eEmit)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the emit compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eEmit)] = result.value;

        _brain.device.destroy(shaderModule);
    }

    { // simulate
        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        std::array<vk::DescriptorSetLayout, 3> layouts = { _brain.bindlessLayout, _particlesBuffersDescriptorSetLayout, _instancesDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

        vk::PushConstantRange pcRange = {};
        pcRange.stageFlags = vk::ShaderStageFlagBits::eCompute;
        pcRange.offset = 0;
        pcRange.size = sizeof(_simulatePushConstant);

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)]),
            "Failed creating simulate pipeline layout!");

        auto byteCode = shader::ReadFile("shaders/bin/simulate.comp.spv");

        vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = "main";

        vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eSimulate)];
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        auto result = _brain.device.createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the simulate compute pipeline!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eSimulate)] = result.value;

        _brain.device.destroy(shaderModule);
    }

    { // instanced rendering (billboard)
        std::array<vk::DescriptorSetLayout, 3> descriptorLayouts = { _brain.bindlessLayout, _instancesDescriptorSetLayout, _camera.DescriptorSetLayout() };

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.setLayoutCount = descriptorLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = descriptorLayouts.data();

        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderInstanced)]),
            "Failed creating particle rendering pipeline layout!");

        auto vertByteCode = shader::ReadFile("shaders/bin/billboard.vert.spv");
        auto fragByteCode = shader::ReadFile("shaders/bin/particle.frag.spv");

        vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);
        vk::ShaderModule fragModule = shader::CreateShaderModule(fragByteCode, _brain.device);

        vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo {};
        vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageCreateInfo.module = vertModule;
        vertShaderStageCreateInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo {};
        fragShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageCreateInfo.module = fragModule;
        fragShaderStageCreateInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
        inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = vk::False;

        std::array<vk::DynamicState, 2> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
        dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {};
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
        rasterizationStateCreateInfo.depthClampEnable = vk::False;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
        rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
        rasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
        rasterizationStateCreateInfo.depthBiasEnable = vk::False;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
        multisampleStateCreateInfo.sampleShadingEnable = vk::False;
        multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
        multisampleStateCreateInfo.alphaToOneEnable = vk::False;

        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {};
        colorBlendAttachmentState.blendEnable = vk::False;
        colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.logicOpEnable = vk::False;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
        depthStencilStateCreateInfo.depthTestEnable = false;
        depthStencilStateCreateInfo.depthWriteEnable = false;

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

        auto& pipelineCreateInfo = structureChain.get<vk::GraphicsPipelineCreateInfo>();
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = _pipelineLayouts[static_cast<uint32_t>(ShaderStages::eRenderInstanced)];
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = nullptr;
        pipelineCreateInfo.basePipelineIndex = -1;

        auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
        pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
        vk::Format format = _swapChain.GetFormat();
        pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

        pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

        auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
        util::VK_ASSERT(result.result, "Failed creating the particle rendering pipeline layout!");
        _pipelines[static_cast<uint32_t>(ShaderStages::eRenderInstanced)] = result.value;

        _brain.device.destroy(vertModule);
        _brain.device.destroy(fragModule);
    }
}

void ParticlePipeline::CreateDescriptorSetLayouts()
{
    { // Particle Storage Buffers
        std::array<vk::DescriptorSetLayoutBinding, 5> bindings {};
        for (size_t i = 0; i < bindings.size(); i++)
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
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings {};

        for (size_t i = 0; i < bindings.size(); i++)
        {
            vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[i] };
            descriptorSetLayoutBinding.binding = i;
            descriptorSetLayoutBinding.descriptorCount = 1;
            descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
        }

        vk::DescriptorSetLayoutCreateInfo createInfo {};
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();

        util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_instancesDescriptorSetLayout),
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
        UpdateParticleBuffersDescriptorSets();
    }

    { // Particle and culled Instances Storage Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_instancesDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Particle Instances Storage Buffer descriptor sets!");

        _instancesDescriptorSet = descriptorSets[0];
        UpdateParticleInstancesBuffersDescriptorSets();
    }

    { // Emitter Uniform Buffers
        vk::DescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.descriptorPool = _brain.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_emittersBufferDescriptorSetLayout;

        std::array<vk::DescriptorSet, 1> descriptorSets;
        util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Emitter Uniform Buffer descriptor sets!");

        _emittersDescriptorSet = descriptorSets[0];
        UpdateEmittersBuffersDescriptorSets();
    }
}

void ParticlePipeline::UpdateParticleBuffersDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 5> descriptorWrites {};

    // Particle SSB (binding = 0)
    uint32_t index = static_cast<uint32_t>(ParticleBufferUsage::eParticle);
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
    index = static_cast<uint32_t>(ParticleBufferUsage::eAliveNew);
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
    index = static_cast<uint32_t>(ParticleBufferUsage::eAliveCurrent);
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
    index = static_cast<uint32_t>(ParticleBufferUsage::eDead);
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
    index = static_cast<uint32_t>(ParticleBufferUsage::eCounter);
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

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePipeline::UpdateParticleInstancesBuffersDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 2> descriptorWrites {};

    // Particle Instances (binding = 0)
    vk::DescriptorBufferInfo particleInstancesBufferInfo {};
    particleInstancesBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_particleInstancesBuffer)->buffer;
    particleInstancesBufferInfo.offset = 0;
    particleInstancesBufferInfo.range = sizeof(ParticleInstance) * MAX_PARTICLES;
    vk::WriteDescriptorSet& particleInstancesBufferWrite { descriptorWrites[0] };
    particleInstancesBufferWrite.dstSet = _instancesDescriptorSet;
    particleInstancesBufferWrite.dstBinding = 0;
    particleInstancesBufferWrite.dstArrayElement = 0;
    particleInstancesBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    particleInstancesBufferWrite.descriptorCount = 1;
    particleInstancesBufferWrite.pBufferInfo = &particleInstancesBufferInfo;

    // Culled Instance (binding = 1)
    vk::DescriptorBufferInfo culledInstanceBufferInfo {};
    culledInstanceBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_culledIndicesBuffer)->buffer;
    culledInstanceBufferInfo.offset = 0;
    culledInstanceBufferInfo.range = sizeof(uint32_t) * (MAX_PARTICLES + 1);
    vk::WriteDescriptorSet& culledInstanceBufferWrite { descriptorWrites[1] };
    culledInstanceBufferWrite.dstSet = _instancesDescriptorSet;
    culledInstanceBufferWrite.dstBinding = 1;
    culledInstanceBufferWrite.dstArrayElement = 0;
    culledInstanceBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    culledInstanceBufferWrite.descriptorCount = 1;
    culledInstanceBufferWrite.pBufferInfo = &culledInstanceBufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void ParticlePipeline::UpdateEmittersBuffersDescriptorSets()
{
    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    // Emitter UB (binding = 0)
    vk::DescriptorBufferInfo emitterBufferInfo {};
    emitterBufferInfo.buffer = _brain.GetBufferResourceManager().Access(_emittersBuffer)->buffer;
    emitterBufferInfo.offset = 0;
    emitterBufferInfo.range = sizeof(Emitter) * MAX_EMITTERS;
    vk::WriteDescriptorSet& emitterBufferWrite { descriptorWrites[0] };
    emitterBufferWrite.dstSet = _emittersDescriptorSet;
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
        _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eParticle)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particles, 0, _brain.GetBufferResourceManager().Access(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eParticle)])->buffer);
    }

    { // Alive and Dead SSBs
        vk::DeviceSize indexBufferSize = sizeof(uint32_t) * MAX_PARTICLES;

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
        _particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eCounter)] = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleCounters, 0, _brain.GetBufferResourceManager().Access(_particlesBuffers[static_cast<uint32_t>(ParticleBufferUsage::eCounter)])->buffer);
    }

    { // Particle Instances SSB
        std::vector<ParticleInstance> particleInstances(MAX_PARTICLES);
        vk::DeviceSize particleInstancesBufferSize = sizeof(ParticleInstance) * MAX_PARTICLES;

        BufferCreation creation {};
        creation.SetName("Particle Instances SSB")
            .SetSize(particleInstancesBufferSize)
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _particleInstancesBuffer = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(particleInstances, 0, _brain.GetBufferResourceManager().Access(_particleInstancesBuffer)->buffer);
    }

    { // Culled Instance SSB
        std::vector<uint32_t> culledInstance(MAX_PARTICLES + 1);
        vk::DeviceSize culledInstanceBufferSize = sizeof(uint32_t) * (MAX_PARTICLES + 1);

        BufferCreation creation {};
        creation.SetName("Culled Instance SSB")
            .SetSize(culledInstanceBufferSize)
            .SetIsMappable(true)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
        _culledIndicesBuffer = _brain.GetBufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(culledInstance, 0, _brain.GetBufferResourceManager().Access(_culledIndicesBuffer)->buffer);
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
        _emittersBuffer = _brain.GetBufferResourceManager().Create(creation);
    }

    { // Emitter Staging buffer
        vk::DeviceSize bufferSize = MAX_EMITTERS * sizeof(Emitter);
        util::CreateBuffer(_brain, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, _stagingBuffer, true, _stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Staging buffer");
    }
}
