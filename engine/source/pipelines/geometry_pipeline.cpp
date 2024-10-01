#include "pipelines/geometry_pipeline.hpp"
#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"

VkDeviceSize align(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

GeometryPipeline::GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, vk::DescriptorSetLayout materialDescriptorSetLayout,
    const CameraStructure& camera)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _camera(camera)
{
    CreateDescriptorSetLayout();
    CreateUniformBuffers();
    CreateDescriptorSets();
    CreatePipeline(materialDescriptorSetLayout);
}

GeometryPipeline::~GeometryPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    for (size_t i = 0; i < _frameData.size(); ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _frameData[i].uniformBufferAllocation);
        vmaDestroyBuffer(_brain.vmaAllocator, _frameData[i].uniformBuffer, _frameData[i].uniformBufferAllocation);
    }
    _brain.device.destroy(_descriptorSetLayout);
}

void GeometryPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene, const BatchBuffer& batchBuffer)
{
    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos[i] };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.clearValue.color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    colorAttachmentInfos[0].imageView = _brain.ImageResourceManager().Access(_gBuffers.AlbedoM())->view;
    colorAttachmentInfos[1].imageView = _brain.ImageResourceManager().Access(_gBuffers.NormalR())->view;
    colorAttachmentInfos[2].imageView = _brain.ImageResourceManager().Access(_gBuffers.EmissiveAO())->view;
    colorAttachmentInfos[3].imageView = _brain.ImageResourceManager().Access(_gBuffers.Position())->view;

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _brain.ImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    glm::uvec2 displaySize = _gBuffers.Size();
    renderingInfo.renderArea.extent = vk::Extent2D { displaySize.x, displaySize.y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    util::BeginLabel(commandBuffer, "Geometry pass", glm::vec3 { 6.0f, 214.0f, 160.0f } / 255.0f, _brain.dldi);

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.setViewport(0, 1, &_gBuffers.Viewport());
    commandBuffer.setScissor(0, 1, &_gBuffers.Scissor());

    std::vector<glm::mat4> transforms;
    for (auto& gameObject : scene.gameObjects)
    {
        for (auto& node : gameObject.model->hierarchy.allNodes)
        {
            transforms.emplace_back(gameObject.transform * node.transform);
        }
    }
    UpdateUniformData(currentFrame, transforms, scene.camera);

    uint32_t counter = 0;
    for (auto& gameObject : scene.gameObjects)
    {
        for (size_t i = 0; i < gameObject.model->hierarchy.allNodes.size(); ++i, ++counter)
        {
            const auto& node = gameObject.model->hierarchy.allNodes[i];

            for (const auto& primitive : node.mesh->primitives)
            {
                assert(primitive.material && "There should always be a material available.");
                const MaterialHandle& material = *primitive.material;

                uint32_t dynamicOffset = static_cast<uint32_t>(counter * sizeof(UBO));

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_brain.bindlessSet, 0, nullptr);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1,
                    &_frameData[currentFrame].descriptorSet, 1, &dynamicOffset);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, 1,
                    &_camera.descriptorSets[currentFrame], 0, nullptr);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 3, 1, &material.descriptorSet, 0,
                    nullptr);

                vk::Buffer vertexBuffers[] = { batchBuffer.VertexBuffer() };
                vk::DeviceSize offsets[] = { 0 };
                commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
                commandBuffer.bindIndexBuffer(batchBuffer.IndexBuffer(), 0, batchBuffer.IndexType());

                commandBuffer.drawIndexed(primitive.count, 1, primitive.indexOffset, primitive.vertexOffset, 0);
            }
        }
    }

    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void GeometryPipeline::CreatePipeline(vk::DescriptorSetLayout materialDescriptorSetLayout)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 4> layouts = { _brain.bindlessLayout, _descriptorSetLayout, _camera.descriptorSetLayout,
        materialDescriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed creating geometry pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/geom-v.spv");
    auto fragByteCode = shader::ReadFile("shaders/geom-f.spv");

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

    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributes = Vertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributes.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributes.data();

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
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
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

    std::array<vk::PipelineColorBlendAttachmentState, DEFERRED_ATTACHMENT_COUNT> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo {};
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
    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    std::array<vk::Format, DEFERRED_ATTACHMENT_COUNT> formats {};
    std::fill(formats.begin(), formats.end(), GBuffers::GBufferFormat());
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = DEFERRED_ATTACHMENT_COUNT;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = formats.data();
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = _gBuffers.DepthFormat();

    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfoKhr;
    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the geometry pipeline layout!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void GeometryPipeline::CreateDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_descriptorSetLayout),
        "Failed creating geometry descriptor set layout!");
}

void GeometryPipeline::CreateDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _frameData[i].descriptorSet = descriptorSets[i];
        UpdateGeometryDescriptorSet(i);
    }
}

void GeometryPipeline::UpdateGeometryDescriptorSet(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = _frameData[frameIndex].uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UBO);

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _frameData[frameIndex].descriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GeometryPipeline::CreateUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UBO) * MAX_MESHES;

    for (size_t i = 0; i < _frameData.size(); ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            _frameData[i].uniformBuffer, true, _frameData[i].uniformBufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _frameData[i].uniformBufferAllocation, &_frameData[i].uniformBufferMapped),
            "Failed mapping memory for UBO!");
    }
}

void GeometryPipeline::UpdateUniformData(uint32_t currentFrame, const std::vector<glm::mat4> transforms, const Camera& camera)
{
    std::array<UBO, MAX_MESHES> ubos;
    for (size_t i = 0; i < std::min(transforms.size(), ubos.size()); ++i)
    {
        ubos[i].model = transforms[i];
    }

    memcpy(_frameData[currentFrame].uniformBufferMapped, ubos.data(), ubos.size() * sizeof(UBO));
}
