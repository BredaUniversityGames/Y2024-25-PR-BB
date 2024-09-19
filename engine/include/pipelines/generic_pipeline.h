//
// Created by luuk on 18-9-2024.
//
#pragma once
#include <string_view>
#include "include.hpp"

class SwapChain;
class VulkanBrain;

struct PipeLineCreationConfig
{
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info {};
	vk::Viewport viewport{};
	vk::Rect2D scissor{};
	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info {};
	vk::PipelineViewportStateCreateInfo viewport_state_create_info{};
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{};
	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{};
	vk::PipelineColorBlendAttachmentState color_blend_attachment{};
	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{};
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info{};
};


//note that this does not yet handle the pipelinelayout
static PipeLineCreationConfig CreateDefaultPipeLineCreationInfo(const glm::vec2& viewport);

class GenericPipeline
{
public:
	void Initialise(std::string_view shader_vertex_path, std::string_view shader_fragment_path, VulkanBrain&,const PipeLineCreationConfig&,const SwapChain&);
	[[nodiscard]] const PipeLineCreationConfig& getConfig() const {return m_pipeLineCreationInfo;}
	
	vk::Pipeline& operator ()()
	{
			return m_pipeLine;
	}

private:
	vk::Pipeline m_pipeLine {};
	vk::PipelineLayout m_pipeLineLayout {};
	vk::DescriptorSetLayout m_descriptorSetLayout {};
	
	PipeLineCreationConfig m_pipeLineCreationInfo {};
};

