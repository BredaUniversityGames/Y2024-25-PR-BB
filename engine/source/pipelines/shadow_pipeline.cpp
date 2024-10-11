#include "pipelines/shadow_pipeline.hpp"

#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"
#include "gpu_scene.hpp"

ShadowPipeline::ShadowPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const GPUScene& gpuScene)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _shadowCamera(_brain)
{
    CreatePipeline(gpuScene);
}

ShadowPipeline::~ShadowPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}

void ShadowPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame,
    const RenderSceneDescription& scene)
{
    _shadowCamera.Update(currentFrame, scene.sceneDescription.directionalLight.camera);

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _brain.GetImageResourceManager().Access(_gBuffers.Shadow())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { 4096, 4096 }; // Shadow map size
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;

    util::BeginLabel(commandBuffer, "Shadow pass", glm::vec3 { 0.0f, 1.0f, 1.0f }, _brain.dldi);

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    // Set viewport and scissor for shadow map size
    vk::Viewport viewport { 0.0f, 0.0f, 4096.0f, 4096.0f, 0.0f, 1.0f };
    vk::Rect2D scissor { vk::Offset2D { 0, 0 }, vk::Extent2D { 4096, 4096 } };
    commandBuffer.setViewport(0, 1, &viewport);
    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { scene.gpuScene.GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _shadowCamera.DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { scene.gpuScene.GetSceneDescriptorSet(currentFrame) }, {});

    vk::Buffer vertexBuffer = _brain.GetBufferResourceManager().Access(scene.batchBuffer.VertexBuffer())->buffer;
    vk::Buffer indexBuffer = _brain.GetBufferResourceManager().Access(scene.batchBuffer.IndexBuffer())->buffer;
    vk::Buffer indirectDrawBuffer = _brain.GetBufferResourceManager().Access(scene.gpuScene.IndirectDrawBuffer(currentFrame))->buffer;
    vk::Buffer indirectCountBuffer = _brain.GetBufferResourceManager().Access(scene.gpuScene.IndirectCountBuffer(currentFrame))->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.batchBuffer.IndexType());
    uint32_t indirectCountOffset = scene.gpuScene.IndirectCountOffset();
    commandBuffer.drawIndexedIndirectCountKHR(indirectDrawBuffer, 0, indirectCountBuffer, indirectCountOffset, scene.gpuScene.DrawCount(), sizeof(vk::DrawIndexedIndirectCommand), _brain.dldi);
    _brain.drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void ShadowPipeline::CreatePipeline(const GPUScene& gpuScene)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 3> layouts = { gpuScene.GetObjectInstancesDescriptorSetLayout(), CameraResource::DescriptorSetLayout(), gpuScene.GetSceneDescriptorSetLayout() };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed to create shadow pipeline layout!");

    // Load shaders (simple vertex shader for depth only)
    auto vertByteCode = shader::ReadFile("shaders/bin/shadow.vert.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);

    vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo {};
    vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo };

    // Vertex input (same as GeometryPipeline)
    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributes = Vertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributes.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;

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
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizationStateCreateInfo.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

    auto& pipelineCreateInfo = structureChain.get<vk::GraphicsPipelineCreateInfo>();
    pipelineCreateInfo.stageCount = 1;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = _pipelineLayout;

    // Use dynamic rendering
    auto& pipelineRenderingCreateInfoKhr = structureChain.get<vk::PipelineRenderingCreateInfoKHR>();
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = _brain.GetImageResourceManager().Access(_gBuffers.Shadow())->format;

    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed to create shadow pipeline!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
}
