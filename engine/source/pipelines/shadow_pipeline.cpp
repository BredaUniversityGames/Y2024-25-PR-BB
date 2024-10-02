#include "pipelines/shadow_pipeline.hpp"

#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"

ShadowPipeline::ShadowPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, GeometryPipeline& geometryPipeline)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _camera(camera)
    , _descriptorSetLayout(geometryPipeline.DescriptorSetLayout())
    , _frameData(geometryPipeline.GetFrameData())
{
    CreatePipeline();
}

ShadowPipeline::~ShadowPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}

void ShadowPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame,
    const SceneDescription& scene, const BatchBuffer& batchBuffer)
{
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

    uint32_t counter = 0;
    for (auto& gameObject : scene.gameObjects)
    {
        for (size_t i = 0; i < gameObject.model->hierarchy.allNodes.size(); ++i, ++counter)
        {
            const auto& node = gameObject.model->hierarchy.allNodes[i];

            for (const auto& primitive : node.mesh->primitives)
            {
                uint32_t dynamicOffset = static_cast<uint32_t>(counter * sizeof(InstanceData));

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1,
                    &_frameData[currentFrame].descriptorSet, 1, &dynamicOffset);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1,
                    &_camera.descriptorSets[currentFrame], 0, nullptr);

                vk::Buffer vertexBuffers[] = { batchBuffer.VertexBuffer() };
                vk::DeviceSize offsets[] = { 0 };
                commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
                commandBuffer.bindIndexBuffer(batchBuffer.IndexBuffer(), 0, batchBuffer.IndexType());

                commandBuffer.drawIndexed(primitive.count, 1, primitive.indexOffset, primitive.vertexOffset, 0);
                _brain.drawStats.indexCount += primitive.count;
                _brain.drawStats.drawCalls++;
            }
        }
    }

    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void ShadowPipeline::CreatePipeline()
{
    // Pipeline layout with two descriptor sets: object data and light camera data
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 2> layouts = { _descriptorSetLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed to create shadow pipeline layout!");

    // Load shaders (simple vertex shader for depth only)
    auto vertByteCode = shader::ReadFile("shaders/shadow-v.spv");

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

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo {};
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
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo {};
    pipelineRenderingCreateInfo.depthAttachmentFormat = _brain.GetImageResourceManager().Access(_gBuffers.Shadow())->format;

    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed to create shadow pipeline!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
}
