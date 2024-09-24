//
// Created by luuk on 16-9-2024.
//

#include "include/UserInterfaceSystem.h"


#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"
#include "fonts.h"

void UserInterfaceSystem::Update(const InputManager& input)
{
	for (auto& i : m_ui_sub_systems_)
	{
		i.second->Update(ui_Registry,input);
	}
}

void UserInterfaceSystem::Render(vk::CommandBuffer commandBuffer, uint32_t currentFrame,
	const ResourceHandle<Image>& _hdrTarget,  UIPipeLine& pipeline, const glm::mat4& cameraMatrix)
{

			
	vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo{};
	finalColorAttachmentInfo.imageView =  pipeline.m_brain.ImageResourceManager().Access(_hdrTarget)->views[0];
	finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
	finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
	finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;

	vk::RenderingInfoKHR renderingInfo{};
	renderingInfo.renderArea.extent = vk::Extent2D{ pipeline.m_brain.ImageResourceManager().Access(_hdrTarget)->width,pipeline.m_brain.ImageResourceManager().Access(_hdrTarget)->height };
	renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
	renderingInfo.layerCount = 1;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pStencilAttachment = nullptr;

	util::BeginLabel(commandBuffer, "ui", glm::vec3{ 17.0f, 138.0f, 178.0f } / 255.0f, pipeline.m_brain.dldi);
	commandBuffer.beginRenderingKHR(&renderingInfo,pipeline.m_brain.dldi);
	
	for(auto& i : m_ui_sub_systems_)
	{
		i.second->Render(ui_Registry,commandBuffer,currentFrame,_hdrTarget,pipeline,cameraMatrix);
	}
	
	commandBuffer.endRenderingKHR(pipeline.m_brain.dldi);
	util::EndLabel(commandBuffer, pipeline.m_brain.dldi);
	
}



void UIButtonSubSystem::Update(entt::registry& reg,const InputManager& input)
{
	
	for(auto&& [entity, tranform, button] : reg.view<UITransform,Button>().each())
	{
		glm::ivec2 mousePos;
		input.GetMousePosition(mousePos.x, mousePos.y);
		//mouse inside boundary
		if (mousePos.x > static_cast<int>(tranform.Translation.x)
			&& mousePos.x < static_cast<int>(tranform.Translation.x + tranform.Scale.x)
			&& mousePos.y > static_cast<int>(tranform.Translation.y)
			&& mousePos.y < static_cast<int>(tranform.Translation.y + tranform.Scale.y))
		{
			switch (button.m_state)
			{
			case Button::ButtonState::normal:

				button.m_state = Button::ButtonState::hovered;
				button.OnBeginHoverCallBack();
				[[fallthrough]];

			case Button::ButtonState::hovered:
				{
					if (input.IsMouseButtonPressed(InputManager::MouseButton::Left))
					{
						button.m_state = Button::ButtonState::pressed;
						button.OnMouseDownCallBack();
					}
				}
				break;

			case Button::ButtonState::pressed:
				{
					if (input.IsMouseButtonReleased(InputManager::MouseButton::Left))
					{
						button.m_state = Button::ButtonState::normal;
					}
				}
				break;
			}
		}
		else button.m_state = Button::ButtonState::normal;
	}
}

void UIButtonSubSystem::Render(const entt::registry& reg, vk::CommandBuffer commandBuffer, uint32_t currentFrame,
                           const ResourceHandle<Image>& render_target,  UIPipeLine& pipe_line,
                           const glm::mat4& projection)
{/*
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe_line.m_uiPipeLine);


	for (const auto& [entity, transform,button] : reg.view<UITransform,Button>().each())
	{
		
		auto matrix = glm::mat4(1.f);
		matrix = glm::translate(matrix, glm::vec3(transform.Translation.x, transform.Translation.y, 0));
		matrix = glm::scale(matrix, glm::vec3(transform.Scale.x, transform.Scale.y, 1));

		matrix = projection * matrix;
		switch (button.m_state)
		{
		case Button::ButtonState::normal:
			pipe_line.UpdateTexture(button.NormalImage,pipe_line.m_descriptorSet);
			break;

		case Button::ButtonState::hovered:
			pipe_line.UpdateTexture(button.HoveredImage,pipe_line.m_descriptorSet);
			break;

		case Button::ButtonState::pressed:
			pipe_line.UpdateTexture(button.PressedImage,pipe_line.m_descriptorSet);
			break;
		}

		
	
		vkCmdPushConstants(commandBuffer, pipe_line.m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
		                   &matrix);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipe_line.m_pipelineLayout, 0, 1,
										 &pipe_line.m_descriptorSet, 0, nullptr);
//
		commandBuffer.draw(6, 1, 0, 0);
	}*/
}

UIDisplayTextSubSystem::UIDisplayTextSubSystem()
{

}

void UIDisplayTextSubSystem::Render(const entt::registry& reg, vk::CommandBuffer commandBuffer, uint32_t currentFrame,
	const ResourceHandle<Image>& render_target,  UIPipeLine& pipe_line, const glm::mat4& projection)
{
	
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe_line.m_uiPipeLine);
	
	for (const auto& [entity, transform,text] : reg.view<UITransform,Text_Element>().each())
	{
		
		
		auto matrix = glm::mat4(1.f);
		matrix = glm::translate(matrix, glm::vec3(transform.Translation.x, transform.Translation.y, 0));



		

		for(const auto i : text.m_text)
		{

			if(i == ' ')
			{
				matrix = glm::translate(matrix, glm::vec3( text.m_fontSize + text.m_CharacterDistance,0, 0));
				continue;
			}
			if(Font::Characters.find(i) == Font::Characters.end())
			{
				continue;	
			}
			
			Character ch = Font::Characters[i];
			
			

			
			auto finalmatrix = projection *  (glm::scale(matrix, glm::vec3(ch.Size.x,ch.Size.y, 1)));
			vkCmdPushConstants(commandBuffer, pipe_line.m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
				   &finalmatrix);
			
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipe_line.m_pipelineLayout, 0, 1,
							 &ch.DescriptorSet, 0, nullptr);
			
			commandBuffer.draw(6, 1, 0, 0);
			
			matrix = glm::translate(matrix, glm::vec3(ch.Size.x + text.m_CharacterDistance,0, 0));

		
		}
	
	}
}

void UserInterfaceSystem::InitializeDefaultSubSystems()
{
	AddSubSystem<UIDisplayTextSubSystem>();
	AddSubSystem<UIButtonSubSystem>();
}

vk::DescriptorSetLayout UIPipeLine::m_descriptorSetLayout = {};


void UIPipeLine::CreatePipeLine()
{

	auto vertShaderCode = shader::ReadFile("shaders/ui_uber_vert-v.spv");
	auto fragShaderCode = shader::ReadFile("shaders/ui_uber_frag-f.spv");
	m_sampler = util::CreateSampler(m_brain, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 0);

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
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;


	std::array<vk::PushConstantRange,2> bufferRange{};
	bufferRange[0].offset = 0;
	bufferRange[0].size = sizeof(glm::mat4);
	bufferRange[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
	

	
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &UIPipeLine::m_descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	pipelineLayoutInfo.pPushConstantRanges = bufferRange.data(); // Optional

	if (m_brain.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_pipelineLayout) != vk::Result::eSuccess) {
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

	std::array<vk::DescriptorSetLayoutBinding, 1> bindings{};

	vk::DescriptorSetLayoutBinding& samplerLayoutBinding{bindings[0]};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	vk::DescriptorSetLayoutCreateInfo createInfo{};
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();

	util::VK_ASSERT(m_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &m_descriptorSetLayout),
					"Failed creating skydome descriptor set layout!");
	
	vk::DescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.descriptorPool = m_brain.descriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &m_descriptorSetLayout;


	util::VK_ASSERT(m_brain.device.allocateDescriptorSets(&allocateInfo, &m_descriptorSet),
					"Failed allocating descriptor sets!");


	
}

void UIPipeLine::UpdateTexture(ResourceHandle<Image> image,vk::DescriptorSet& set) const
{
	vk::DescriptorImageInfo imageInfo{};
	imageInfo.sampler = *m_sampler;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.imageView = m_brain.ImageResourceManager().Access(image)->views[0];

	vk::WriteDescriptorSet descriptorWrite{};
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

