//
// Created by luuk on 16-9-2024.
//

#pragma once

#include "hdr_target.hpp"
#include "include.hpp"
#include "input_manager.hpp"
class SwapChain;



struct UI_Element
{
	glm::vec2 translation {};
	glm::vec2 scale {};
	
	//todo: update to also accept controller input
	virtual void Evaluate(const InputManager& input) = 0;
};

struct Button : UI_Element
{
	enum class ButtonState
	{
		normal = 0,
		hovered,
		pressed
	};
	
	void Evaluate(const InputManager& input) override;
	std::function<void()> OnBeginHoverCallBack {};
	std::function<void()> OnMouseDownCallBack {};

	ButtonState state = ButtonState::normal;
};

class user_interface {
public:
	user_interface(){}

	void Update(const InputManager& input)
	{
		for (auto& i : m_elements)
		{
			i->Evaluate(input);
		}
	}
	
private:
	//todo: add children
	std::vector<std::unique_ptr<UI_Element>>  m_elements;
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

	void CreateDescriptorSetLayout();
	void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const   HDRTarget& _hdrTarget);
	NON_COPYABLE(UIPipeLine);;
	NON_MOVABLE(UIPipeLine)
	~UIPipeLine();

	VkPipeline m_uiPipeLine;
	VkPipelineLayout m_pipelineLayout;
	const VulkanBrain& m_brain;
	const SwapChain& m_swapChain;
};


