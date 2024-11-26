//
// Created by luuk on 6-10-2024.
//
#include "../public/ui_pipelines.hpp"

#include "swap_chain.hpp"
#include "../../include/vulkan_helper.hpp"
#include "../../include/shaders/shader_loader.hpp"

void UIPipeline::CreatePipeLine()
{
    auto vertShaderCode = shader::ReadFile("shaders/bin/ui.vert.spv");
    auto fragShaderCode = shader::ReadFile("shaders/bin/ui.frag.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertShaderCode, _brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragShaderCode, _brain.device);

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
    pipelineLayoutInfo.pSetLayouts = &_brain.bindlessLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    pipelineLayoutInfo.pPushConstantRanges = bufferRange.data(); // Optional

    if (_brain.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &_pipelineLayout) != vk::Result::eSuccess)
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

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the IBL pipeline!");
    _pipeline = result.value;
    // cleanup
    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}
void UIPipeline::RecordCommands(vk::CommandBuffer commandBuffer, const SwapChain& swapChain, uint32_t swapChainIndex, const glm::mat4& projectionMatrix)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = swapChain.GetImageView(swapChainIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    util::BeginLabel(commandBuffer, "ui pass", glm::vec3 { 239.0f, 71.0f, 111.0f } / 255.0f, _brain.dldi);
    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    UIPushConstants pushConstants;
    for (auto& i : _drawlist)
    {

        pushConstants.quad = i;
        pushConstants.quad.projection = projectionMatrix * pushConstants.quad.projection;
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_brain.bindlessSet, 0, nullptr);

        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(UIPushConstants), &pushConstants);
        commandBuffer.draw(6, 1, 0, 0);
        _brain.drawStats.indexCount += 6;
        _brain.drawStats.drawCalls++;
    }

    commandBuffer.endRenderingKHR(_brain.dldi);
    util::EndLabel(commandBuffer, _brain.dldi);

    _drawlist.clear();
}
UIPipeline::~UIPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}
