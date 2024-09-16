//
// Created by luuk on 16-9-2024.
//

#pragma once

#include "hdr_target.hpp"
#include "include.hpp"

class SwapChain;

class user_interface {
public:
	user_interface(){}
	
	void Draw();

	
};


//todo: move and implement this struct
struct PipeLineConfigInfo
{
};

class UIPipeLine
{
public:

	void CreatePipeLine();
	UIPipeLine(const VulkanBrain& brain, const SwapChain& sc) : m_brain(brain),m_swapChain(sc) {};

	void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const   HDRTarget& _hdrTarget);
	NON_COPYABLE(UIPipeLine);;
	NON_MOVABLE(UIPipeLine)
	~UIPipeLine();

	VkPipeline m_uiPipeLine;
	VkPipelineLayout m_pipelineLayout;
	const VulkanBrain& m_brain;
	const SwapChain& m_swapChain;
};


