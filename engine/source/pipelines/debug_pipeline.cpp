#include "pipelines/debug_pipeline.hpp"

#include "shaders/shader_loader.hpp"
#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "swap_chain.hpp"

#include <imgui_impl_vulkan.h>

DebugPipeline::DebugPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, const SwapChain& swapChain, const GPUScene& gpuScene)
    : _brain(brain)
    , _gBuffers(gBuffers)
    , _camera(camera)
    , _descriptorSetLayout(gpuScene.GetSceneDescriptorSetLayout())
    , _swapChain(swapChain)

{

    _linesData.reserve(2048); // pre allocate some memory
    CreateVertexBuffer();
    CreatePipeline();
}

DebugPipeline::~DebugPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    _brain.GetBufferResourceManager().Destroy(_vertexBuffer);
}

void DebugPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex)
{

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = _swapChain.GetImageView(swapChainIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
    finalColorAttachmentInfo.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

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
    renderingInfo.renderArea.extent = _swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    util::BeginLabel(commandBuffer, "Debug render pass", glm::vec3 { 0.0f, 1.0f, 1.0f }, _brain.dldi);

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.setViewport(0, 1, &_gBuffers.Viewport());
    commandBuffer.setScissor(0, 1, &_gBuffers.Scissor());

    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1,
        &_camera.descriptorSets[currentFrame], 0, nullptr);

    UpdateVertexData(currentFrame);

    // to draw lines
    // Bind the vertex buffer

    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_vertexBuffer);
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, offsets);

    // Draw the lines
    commandBuffer.draw(static_cast<uint32_t>(_linesData.size()), 1, 0, 0);
    _brain.drawStats.drawCalls++;
    _brain.drawStats.debugLines = static_cast<uint32_t>(_linesData.size() / 2);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    commandBuffer.endRenderingKHR(_brain.dldi);

    util::EndLabel(commandBuffer, _brain.dldi);
}

void DebugPipeline::CreatePipeline()
{
    // Pipeline layout with two descriptor sets: object data and light camera data
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 2> layouts = { _descriptorSetLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
        "Failed to create debug rendering pipeline layout!");

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
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    vk::Format format = _swapChain.GetFormat();
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    pipelineRenderingCreateInfo.depthAttachmentFormat = _gBuffers.DepthFormat();
    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed to create debug render pipeline!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}
void DebugPipeline::CreateVertexBuffer()
{
    /*vk::DeviceSize bufferSize = sizeof(glm::vec3) * 2 * 1024 * 2048; // big enough we probably won't need to resize

    for (size_t i = 0; i < _frameData.size(); ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer,
            _frameData[i].vertexBuffer, true, _frameData[i].vertexBufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _frameData[i].vertexBufferAllocation, &_frameData[i].vertexBufferMapped),
            "Failed mapping memory for UBO!");
    }*/

    vk::DeviceSize bufferSize = sizeof(glm::vec3) * 2 * 1024 * 2048;
    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(bufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(true)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Debug vertex buffer");

    _vertexBuffer = _brain.GetBufferResourceManager().Create(vertexBufferCreation);
}

void DebugPipeline::UpdateVertexData(uint32_t currentFrame)
{
    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_vertexBuffer);
    memcpy(buffer->mappedPtr, _linesData.data(), _linesData.size() * sizeof(glm::vec3));
}
