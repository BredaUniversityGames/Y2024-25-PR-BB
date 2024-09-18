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
	UI_Element() = default;
	UI_Element(const glm::vec2& translation, const glm::vec2& scale) : Translation(translation), Scale(scale) {}
	glm::vec2 Translation {};
	glm::vec2 Scale {};
	
	//todo: update to also accept controller input
	virtual void Evaluate(const InputManager& input) {}
	virtual void Render() = 0;
};


class Text_Element : UI_Element
{
public:
	Text_Element(const glm::vec2& translation) : UI_Element(translation, ) {}

	int GetFontSize() const {return m_fontSize;}
	void SetFontSize(int new_size){m_fontSize = new_size;}

	bool GetTextWrap() const {return m_textWrap;}
	void SetTextWrap(bool wrap) {m_textWrap = wrap;}

	void Render() override;
protected:
	bool m_textWrap = true;
	int m_fontSize = 10;
	std::string m_text = "placeholder";
};

struct Button : UI_Element
{
	enum class ButtonState
	{
		normal = 0,
		hovered,
		pressed
	};

	Button() = default;
	Button(const glm::vec2& translation, const glm::vec2& scale) : UI_Element(translation, scale) {}
	
	void Evaluate(const InputManager& input) override;
	std::function<void()> OnBeginHoverCallBack {};
	std::function<void()> OnMouseDownCallBack {};

	ButtonState state = ButtonState::normal;
};

class user_interface {
public:
	
	void Update(const InputManager& input);
	
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


