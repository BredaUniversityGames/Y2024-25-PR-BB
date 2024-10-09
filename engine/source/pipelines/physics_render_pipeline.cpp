#include "pipelines/physics_render_pipeline.hpp"

#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"

PhysicsRenderPipeline::PhysicsRenderPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, GeometryPipeline& geometryPipeline)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _camera(camera)
    , _descriptorSetLayout(geometryPipeline.DescriptorSetLayout())
    , _geometryFrameData(geometryPipeline.GetFrameData())
{

    linePoints.push_back(glm::vec3 { 0.0f, 0.0f, 0.0f });
    linePoints.push_back(glm::vec3 { 0.0f, 2.0f, 0.0f });

    CreateVertexBuffer();
    CreatePipeline();
}

PhysicsRenderPipeline::~PhysicsRenderPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    for (size_t i = 0; i < _frameData.size(); ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _frameData[i].vertexBufferAllocation);
        vmaDestroyBuffer(_brain.vmaAllocator, _frameData[i].vertexBuffer, _frameData[i].vertexBufferAllocation);
    }
}

void PhysicsRenderPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame)
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

    colorAttachmentInfos[0].imageView = _brain.GetImageResourceManager().Access(_gBuffers.AlbedoM())->view;
    colorAttachmentInfos[1].imageView = _brain.GetImageResourceManager().Access(_gBuffers.NormalR())->view;
    colorAttachmentInfos[2].imageView = _brain.GetImageResourceManager().Access(_gBuffers.EmissiveAO())->view;
    colorAttachmentInfos[3].imageView = _brain.GetImageResourceManager().Access(_gBuffers.Position())->view;

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
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    util::BeginLabel(commandBuffer, "Physics render pass", glm::vec3 { 0.0f, 1.0f, 1.0f }, _brain.dldi);

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.setViewport(0, 1, &_gBuffers.Viewport());
    commandBuffer.setScissor(0, 1, &_gBuffers.Scissor());

    // Bind descriptor sets
    uint32_t dynamicOffset = static_cast<uint32_t>(sizeof(UBO));
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1,
        &_geometryFrameData[currentFrame].descriptorSet, 1, &dynamicOffset);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1,
        &_camera.descriptorSets[currentFrame], 0, nullptr);

    UpdateVertexData(currentFrame);

    // to draw lines
    // Bind the vertex buffer
    vk::Buffer vertexBuffers[] = { _frameData[currentFrame].vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    // Draw the lines
    commandBuffer.draw(static_cast<uint32_t>(linePoints.size()), 1, 0, 0);
    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void PhysicsRenderPipeline::CreatePipeline()
{
    // Pipeline layout with two descriptor sets: object data and light camera data
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 2> layouts = { _descriptorSetLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed to create physics rendering pipeline layout!");

    // Load shaders (simple vertex shader for depth only)
    auto vertByteCode = shader::ReadFile("shaders/physics-v.spv");
    auto fragByteCode = shader::ReadFile("shaders/physics-f.spv");

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

    // Vertex input
    auto bindingDesc = LineVertex::GetBindingDescription();
    auto attributes = LineVertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1; // 1
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributes.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eLineList;

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
    pipelineCreateInfo.stageCount = 2;
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
    std::array<vk::Format, DEFERRED_ATTACHMENT_COUNT> formats {};
    std::fill(formats.begin(), formats.end(), GBuffers::GBufferFormat());
    pipelineRenderingCreateInfo.colorAttachmentCount = DEFERRED_ATTACHMENT_COUNT;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = formats.data();
    pipelineRenderingCreateInfo.depthAttachmentFormat = _gBuffers.DepthFormat();
    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed to create physics render pipeline!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}
void PhysicsRenderPipeline::CreateVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(glm::vec3) * linePoints.size();

    for (size_t i = 0; i < _frameData.size(); ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer,
            _frameData[i].vertexBuffer, true, _frameData[i].vertexBufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _frameData[i].vertexBufferAllocation, &_frameData[i].vertexBufferMapped),
            "Failed mapping memory for UBO!");
    }
}

void PhysicsRenderPipeline::UpdateVertexData(uint32_t currentFrame)
{

    memcpy(_frameData[currentFrame].vertexBufferMapped, linePoints.data(), linePoints.size() * sizeof(glm::vec3));
}
