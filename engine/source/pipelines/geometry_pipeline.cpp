#include "pipelines/geometry_pipeline.hpp"
#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"
#include "gpu_scene.hpp"

VkDeviceSize align(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

GeometryPipeline::GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, const GPUScene& gpuScene)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _camera(camera)
{
    CreatePipeline(gpuScene);
    CreateCullingPipeline();
}

GeometryPipeline::~GeometryPipeline()
{
    _brain.device.destroy(_cullingPipeline);
    _brain.device.destroy(_cullingPipelineLayout);
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}

void GeometryPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const BatchBuffer& batchBuffer)
{
    _drawCommands.clear();
    uint32_t counter = 0;
    for (auto& gameObject : scene.sceneDescription.gameObjects)
    {
        for (size_t i = 0; i < gameObject.model->hierarchy.allNodes.size(); ++i, ++counter)
        {
            const auto& node = gameObject.model->hierarchy.allNodes[i];

            auto mesh = _brain.GetMeshResourceManager().Access(node.mesh);
            for (const auto& primitive : mesh->primitives)
            {
                _brain.drawStats.indexCount += primitive.count;

                _drawCommands.emplace_back(vk::DrawIndexedIndirectCommand {
                    .indexCount = primitive.count,
                    .instanceCount = 1,
                    .firstIndex = primitive.indexOffset,
                    .vertexOffset = static_cast<int32_t>(primitive.vertexOffset),
                    .firstInstance = 0 });
            }
        }
    }

    batchBuffer.WriteDraws(_drawCommands, currentFrame);

    const uint32_t localSize = 16;
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
    commandBuffer.dispatch(_drawCommands.size() / localSize, 0, 0);

    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos[i] };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    }

    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        colorAttachmentInfos[i].imageView = _brain.GetImageResourceManager().Access(_gBuffers.Attachments()[i])->view;

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _brain.GetImageResourceManager().Access(_gBuffers.Depth())->view;
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

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_brain.bindlessSet, 0, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1, &scene.gpuScene.GetObjectInstancesDescriptorSet(currentFrame), 0, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, 1, &_camera.descriptorSets[currentFrame], 0, nullptr);

    vk::Buffer vertexBuffers[] = { batchBuffer.VertexBuffer() };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(batchBuffer.IndexBuffer(), 0, batchBuffer.IndexType());
    commandBuffer.drawIndexedIndirect(batchBuffer.IndirectDrawBuffer(currentFrame), 0, _drawCommands.size(), sizeof(vk::DrawIndexedIndirectCommand));
    _brain.drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void GeometryPipeline::CreatePipeline(const GPUScene& gpuScene)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 3> layouts = { _brain.bindlessLayout, gpuScene.GetObjectInstancesDescriptorSetLayout(), _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed creating geometry pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/bin/geom.vert.spv");
    auto fragByteCode = shader::ReadFile("shaders/bin/geom.frag.spv");

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
    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    std::array<vk::Format, DEFERRED_ATTACHMENT_COUNT> formats {};
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        formats[i] = _brain.GetImageResourceManager().Access(_gBuffers.Attachments()[i])->format;

    pipelineRenderingCreateInfoKhr.colorAttachmentCount = DEFERRED_ATTACHMENT_COUNT;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = formats.data();
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = _gBuffers.DepthFormat();

    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the geometry pipeline layout!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void GeometryPipeline::CreateCullingPipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.setLayoutCount = 0;

    _brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_cullingPipelineLayout);

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

    vk::DescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = 0;
    layoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
    layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &layoutBinding;

    _brain.device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_cullingDescriptorSetLayout);

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _cullingDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _cullingDescriptorSet[i] = descriptorSets[i];
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = ;
        bufferInfo.offset = 0;
        bufferInfo.range = vk::WholeSize;

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

        vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
        bufferWrite.dstSet = _cullingDescriptorSet[i];
        bufferWrite.dstBinding = 0;
        bufferWrite.dstArrayElement = 0;
        bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        bufferWrite.descriptorCount = 1;
        bufferWrite.pBufferInfo = &bufferInfo;

        _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
