//
// Created by luuk on 16-9-2024.
//

#include "ui/UserInterfaceSystem.hpp"

#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "ui/fonts.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text_rendering.hpp"

void UpdateUI(const InputManager& input, UIElement* element)
{
    element->Update(input);
}
void RenderUI(UIElement* element, UserInterfaceRenderContext& context, const vk::CommandBuffer& cb, const VulkanBrain& b, SwapChain& swapChain, int swapChainIndex,
    const glm::mat4& projectionMatrix)
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

    cb.beginRenderingKHR(&renderingInfo, b.dldi);

    element->SubmitDrawInfo(context);

    for (auto& i : context.m_UIRenderSystems)
    {
        i.second->Render(cb, projectionMatrix, b);
    }

    cb.endRenderingKHR(b.dldi);
    util::EndLabel(cb, b.dldi);
}

void Canvas::UpdateChildAbsoluteLocations()
{
    {
        for (auto& i : chilren)
        {
            auto relativeLocation = i->GetRelativeLocation();
            switch (i->m_AnchorPoint)
            {
            case AnchorPoint::TOP_LEFT:
                i->UpdateAbsoluteLocation(AbsoluteLocation + relativeLocation);
                break;
            case AnchorPoint::TOP_RIGHT:
                i->UpdateAbsoluteLocation({ AbsoluteLocation.x + Scale.x - relativeLocation.x, AbsoluteLocation.y + relativeLocation.y });
                break;
            case AnchorPoint::BOTTOM_LEFT:
                i->UpdateAbsoluteLocation({ AbsoluteLocation.x + relativeLocation.x, AbsoluteLocation.y + Scale.y - relativeLocation.y });
                break;
            case AnchorPoint::BOTTOM_RIGHT:
                i->UpdateAbsoluteLocation(AbsoluteLocation + Scale - relativeLocation);
                break;
            }

            i->UpdateChildAbsoluteLocations();
        }
    }
}
void Canvas::SubmitDrawInfo(UserInterfaceRenderContext& user_interface_render_context) const
{
    for (auto& i : chilren)
    {
        i->SubmitDrawInfo(user_interface_render_context);
    }
}

void UserInterfaceRenderContext::InitializeDefaultRenderSystems(const UIPipeLine& pipeline)
{
    //   AddRenderingSystem<UITextRenderSystem>(pipeline);
    AddRenderingSystem<UIButtonRenderSystem>(pipeline);
}

vk::DescriptorSetLayout UIPipeLine::m_descriptorSetLayout = {};

void UIPipeLine::CreatePipeLine()
{
    CreateDescriptorSetLayout();
    auto vertShaderCode = shader::ReadFile("shaders/ui_uber_vert-v.spv");
    auto fragShaderCode = shader::ReadFile("shaders/ui_uber_frag-f.spv");
    m_sampler = util::CreateSampler(m_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eLinear, 0);

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertShaderCode, m_brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragShaderCode, m_brain.device);

    // vertex
    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    {
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertModule;
        vertShaderStageInfo.pName = "main";
    }

    // fragment
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    {
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragModule;
        fragShaderStageInfo.pName = "main";
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // temp
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = vk::False;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)1920;
    viewport.height = (float)1080;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { 1920, 1080 };

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = vk::False;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = vk::False;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<vk::PushConstantRange, 2> bufferRange {};
    bufferRange[0].offset = 0;
    bufferRange[0].size = sizeof(glm::mat4);
    bufferRange[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

    bufferRange[1].offset = sizeof(glm::mat4);
    bufferRange[1].size = sizeof(uint32_t);
    bufferRange[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &m_brain.bindlessLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 2; // Optional
    pipelineLayoutInfo.pPushConstantRanges = bufferRange.data(); // Optional

    if (m_brain.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.subpass = 0;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    vk::Format format = vk::Format::eB8G8R8A8Unorm;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

    pipelineInfo.pNext = &pipelineRenderingCreateInfoKhr;
    pipelineInfo.renderPass = nullptr; // Using dynamic rendering.

    if (vkCreateGraphicsPipelines(m_brain.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_uiPipeLine) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // cleanup

    //	vkDestroyShaderModule(m_brain.device, vertModule, nullptr);
    //	vkDestroyShaderModule(m_brain.device, fragModule, nullptr);
}
void UIPipeLine::CreateDescriptorSetLayout()
{
}

void UIPipeLine::UpdateTexture(ResourceHandle<Image> image, vk::DescriptorSet& set) const
{
    vk::DescriptorImageInfo imageInfo {};
    imageInfo.sampler = *m_sampler;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = m_brain.ImageResourceManager().Access(image)->views[0];

    vk::WriteDescriptorSet descriptorWrite {};
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    m_brain.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

UIPipeLine::~UIPipeLine()
{
}
