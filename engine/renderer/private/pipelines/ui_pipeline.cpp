#include "pipelines/ui_pipeline.hpp"

#include "gpu_scene.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"

#include <graphics_context.hpp>
#include <vulkan_context.hpp>

#include "vulkan_helper.hpp"

// TODO: UPDATE WITH THE NEW RENDER SYSTEMS
void UIPipeline::CreatePipeLine()
{
    auto vertShaderCode = shader::ReadFile("shaders/bin/ui.vert.spv");
    auto fragShaderCode = shader::ReadFile("shaders/bin/ui.frag.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertShaderCode, _context.VulkanContext()->Device());
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragShaderCode, _context.VulkanContext()->Device());

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertModule,
        .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragModule,
        .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .dynamicStateCount = dynamicStates.size(),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo {
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = vk::False,
        .alphaToOneEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentstate {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentstate;

    std::array<vk::PushConstantRange, 1> bufferRange {};
    bufferRange[0].offset = 0;
    bufferRange[0].size = sizeof(UIPushConstants);
    bufferRange[0].stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    // pipelineLayoutInfo.pSetLayouts = _context.BindlessLayout();
    pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    pipelineLayoutInfo.pPushConstantRanges = bufferRange.data(); // Optional

    if (_context.VulkanContext()->Device().createPipelineLayout(&pipelineLayoutInfo, nullptr, &_pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineInfo.pViewportState = &viewportStateCreateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.subpass = 0;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    vk::Format format = vk::Format::eB8G8R8A8Unorm;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

    pipelineInfo.pNext = &pipelineRenderingCreateInfoKhr;
    pipelineInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _context.VulkanContext()->Device().createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the IBL pipeline!");
    _pipeline = result.value;
    // cleanup
    _context.VulkanContext()->Device().destroy(vertModule);
    _context.VulkanContext()->Device().destroy(fragModule);
}
void UIPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = _swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    util::BeginLabel(commandBuffer, "ui pass", glm::vec3 { 239.0f, 71.0f, 111.0f } / 255.0f, _context.VulkanContext()->Dldi());
    commandBuffer.beginRenderingKHR(&renderingInfo, _context.VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    UIPushConstants pushConstants;
    for (auto& i : _drawlist)
    {

        pushConstants.quad = i;
        // pushConstants.quad.projection = projectionMatrix * pushConstants.quad.projection;
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context.BindlessSet() }, {});

        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(UIPushConstants), &pushConstants);
        commandBuffer.draw(6, 1, 0, 0);
        _context.GetDrawStats().Draw(6);
    }

    commandBuffer.endRenderingKHR(_context.VulkanContext()->Dldi());
    util::EndLabel(commandBuffer, _context.VulkanContext()->Dldi());

    _drawlist.clear();
}
UIPipeline::~UIPipeline()
{
    _context.VulkanContext()->Device().destroy(_pipeline);
    _context.VulkanContext()->Device().destroy(_pipelineLayout);
}
