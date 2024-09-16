//
// Created by luuk on 16-9-2024.
//

#include "user_interface.h"

#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"

void user_interface::Draw()
{
}

void Button::Evaluate(const InputManager& input)
{
	glm::ivec2 mousePos;
	input.GetMousePosition(mousePos.x, mousePos.y);


	//mouse inside boundary
	if (mousePos.x > static_cast<int>(translation.x)
		&& mousePos.x < static_cast<int>(translation.x + scale.x)
		&& mousePos.y > static_cast<int>(translation.y)
		&& mousePos.y < static_cast<int>(translation.y + scale.y))
	{
		switch (state)
		{
		case ButtonState::normal:

			state = ButtonState::hovered;
			OnBeginHoverCallBack();
			[[fallthrough]];

		case ButtonState::hovered:
			{
				if (input.IsMouseButtonPressed(InputManager::MouseButton::Left))
				{
					state = ButtonState::pressed;
					OnMouseDownCallBack();
				}
			}
			break;

		case ButtonState::pressed:
			{
				if (input.IsMouseButtonReleased(InputManager::MouseButton::Left))
				{
					state = ButtonState::normal;
				}
			}
			break;
		}
	}
	else state = ButtonState::normal;
	
}

void UIPipeLine::CreatePipeLine()
{
	auto vertShaderCode = shader::ReadFile("shaders/ui_uber_vert.vert.spv");
	auto fragShaderCode = shader::ReadFile("shaders/ui_uber_frag.frag.spv");
	
	vk::ShaderModule vertModule = shader::CreateShaderModule(vertShaderCode, m_brain.device);
	vk::ShaderModule fragModule = shader::CreateShaderModule(fragShaderCode, m_brain.device);

	//vertex
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	{
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		vertShaderStageInfo.module = vertModule;
		vertShaderStageInfo.pName = "main";
	}

	//fragment
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	{
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragModule;
		fragShaderStageInfo.pName = "main";
	}

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
	
	//temp
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional


	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = vk::False;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)  1920;
	viewport.height = (float) 1080;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = {1920,1080};

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
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

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = vk::False;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
	multisampleStateCreateInfo.alphaToOneEnable = vk::False;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;


	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(glm::mat4);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = &bufferRange; // Optional

	if (vkCreatePipelineLayout(m_brain.device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
	
	VkGraphicsPipelineCreateInfo pipelineInfo{};
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

	
	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr{};
	pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
	vk::Format format = m_swapChain.GetFormat();
	pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

	pipelineInfo.pNext = &pipelineRenderingCreateInfoKhr;
	pipelineInfo.renderPass = nullptr; // Using dynamic rendering.

	
	if (vkCreateGraphicsPipelines(m_brain.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_uiPipeLine) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	
	//cleanup
	
//	vkDestroyShaderModule(m_brain.device, vertModule, nullptr);
//	vkDestroyShaderModule(m_brain.device, fragModule, nullptr);
}

void UIPipeLine::CreateDescriptorSetLayout()
{

}

void UIPipeLine::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame,const   HDRTarget& _hdrTarget)
{
 vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo{};
    finalColorAttachmentInfo.imageView = _hdrTarget.imageViews;
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;

    vk::RenderingInfoKHR renderingInfo{};
    renderingInfo.renderArea.extent = vk::Extent2D{ _hdrTarget.size.x, _hdrTarget.size.y };
    renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    util::BeginLabel(commandBuffer, "ui", glm::vec3{ 17.0f, 138.0f, 178.0f } / 255.0f, m_brain.dldi);
    commandBuffer.beginRenderingKHR(&renderingInfo, m_brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_uiPipeLine);
	auto matrix = glm::mat4(1.f);
	matrix = glm::scale(matrix,glm::vec3(0.2));
	vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &matrix);

	commandBuffer.draw(6,1,0,0);
	

	matrix = glm::translate(matrix,glm::vec3(0,2,0));
	
	vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &matrix);

	commandBuffer.draw(6,1,0,0);

	
    commandBuffer.endRenderingKHR(m_brain.dldi);
    util::EndLabel(commandBuffer, m_brain.dldi);
}

UIPipeLine::~UIPipeLine()
{
}
