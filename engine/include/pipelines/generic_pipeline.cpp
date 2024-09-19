//
// Created by luuk on 18-9-2024.
//

#include "generic_pipeline.h"

#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "shaders/shader_loader.hpp"


void GenericPipeline::Initialise(std::string_view shader_vertex_path, std::string_view shader_fragment_path, VulkanBrain&brain ,  const PipeLineCreationConfig& config,const SwapChain& swap_chain)
{
	
	auto vertShaderCode = shader::ReadFile(shader_vertex_path);
	auto fragShaderCode = shader::ReadFile(shader_fragment_path);
	vk::ShaderModule vertModule = shader::CreateShaderModule(vertShaderCode, brain.device);
	vk::ShaderModule fragModule = shader::CreateShaderModule(fragShaderCode, brain.device);

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

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	if (brain.device.createPipelineLayout(&config.pipeline_layout_create_info, nullptr, &m_pipeLineLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
		
	pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &config.vertex_input_state_create_info;
	pipelineInfo.pInputAssemblyState = &config.input_assembly_state_create_info;
	pipelineInfo.pViewportState = &config.viewport_state_create_info;
	pipelineInfo.pRasterizationState = &config.rasterization_state_create_info;
	pipelineInfo.pMultisampleState = &config.multisample_state_create_info;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &config.color_blend_state_create_info;
	pipelineInfo.pDynamicState = &config.dynamic_state_create_info;
	pipelineInfo.subpass = 0;
	pipelineInfo.layout = m_pipeLineLayout;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;


	vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr{};
	pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
	vk::Format format = swap_chain.GetFormat();
	pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;

	pipelineInfo.pNext = &pipelineRenderingCreateInfoKhr;
	pipelineInfo.renderPass = nullptr; // Using dynamic rendering.


	auto result = brain.device.createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
	m_pipeLine = result.value;
}


PipeLineCreationConfig CreateDefaultPipeLineCreationInfo(const glm::vec2& viewportSize)
{
	PipeLineCreationConfig config{};

	config.vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	config.vertex_input_state_create_info.pVertexBindingDescriptions = nullptr; 
	config.vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	config.vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr; 
	
	config.input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
	config.input_assembly_state_create_info.primitiveRestartEnable = vk::False;

	
	config.viewport.x = 0.0f;
	config.viewport.y = 0.0f;
	config.viewport.width = viewportSize.x;
	config.viewport.height = viewportSize.y;
	config.viewport.minDepth = 0.0f;
	config.viewport.maxDepth = 1.0f;
	
	config.scissor.offset = vk::Offset2D(0, 0);
	config.scissor.extent = vk::Extent2D(1920,1080);

	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	
	config.dynamic_state_create_info.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
	config.dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	config.dynamic_state_create_info.pDynamicStates = dynamicStates.data();
	
	config.viewport_state_create_info.sType =  vk::StructureType::ePipelineViewportStateCreateInfo;
	config.viewport_state_create_info.viewportCount = 1;
	config.viewport_state_create_info.pViewports = &config.viewport;
	config.viewport_state_create_info.scissorCount = 1;
	config.viewport_state_create_info.pScissors = &config.scissor;

	
	config.rasterization_state_create_info.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
	config.rasterization_state_create_info.depthClampEnable = vk::False;
	config.rasterization_state_create_info.rasterizerDiscardEnable = vk::False;
	config.rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
	config.rasterization_state_create_info.lineWidth = 1.0f;
	config.rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eBack;
	config.rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
	config.rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise;
	config.rasterization_state_create_info.depthBiasEnable = vk::False;
	config.rasterization_state_create_info.depthBiasClamp = 0.0f;
	config.rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;


	config.multisample_state_create_info.sType = 	vk::StructureType::ePipelineMultisampleStateCreateInfo;
	config.multisample_state_create_info.sampleShadingEnable = vk::False;
	config.multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
	config.multisample_state_create_info.minSampleShading = 1.0f;
	config.multisample_state_create_info.pSampleMask = nullptr;
	config.multisample_state_create_info.alphaToCoverageEnable = vk::False;
	config.multisample_state_create_info.alphaToOneEnable = vk::False;

	config.color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrc1Alpha;
	config.color_blend_attachment.dstColorBlendFactor =vk::BlendFactor::eOneMinusSrc1Alpha;
	config.color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
	config.color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	config.color_blend_attachment.dstAlphaBlendFactor =  vk::BlendFactor::eZero;
	config.color_blend_attachment.alphaBlendOp =  vk::BlendOp::eAdd;
	config.color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	config.color_blend_attachment.blendEnable = vk::True;

	
	return config;
	
}
