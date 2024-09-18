//
// Created by luuk on 16-9-2024.
//

#pragma once

#include "include.hpp"
#include "input_manager.hpp"
struct CameraUBO;
struct Camera;
class SwapChain;


struct UI_Element
{
	UI_Element() = default;
	UI_Element(const glm::vec2& translation, const glm::vec2& scale) : Translation(translation), Scale(scale) {}
	glm::vec2 Translation {};
	glm::vec2 Scale {};
	
	//todo: update to also accept controller input
	virtual void Evaluate(const InputManager& input) {}
	virtual void Render() {}
};


class Text_Element : UI_Element
{
public:
	Text_Element(const glm::vec2& translation,const glm::vec2& scale) : UI_Element(translation,scale ) {}

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

	ResourceHandle<Image> NormalImage = {};
	ResourceHandle<Image> HoveredImage = {};
	ResourceHandle<Image> PressedImage = {};

	Button() = default;
	Button(const glm::vec2& translation, const glm::vec2& scale) : UI_Element(translation, scale) {}
	
	void Evaluate(const InputManager& input) override;
	void Render() override;
	std::function<void()> OnBeginHoverCallBack {};
	std::function<void()> OnMouseDownCallBack {};

	ButtonState state = ButtonState::normal;
};
class UIPipeLine;

class user_interface {
public:
	
	void Update(const InputManager& input);
	//todo: add children
	std::vector<Button>  m_elements;
	void SubmitCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const   ResourceHandle<Image>& _hdrTarget, const UIPipeLine&,const glm::mat4&);
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
	void UpdateTexture(ResourceHandle<Image> image) const;
	void RecordCommands();
	NON_COPYABLE(UIPipeLine);;
	NON_MOVABLE(UIPipeLine)
	~UIPipeLine();

	VkPipeline m_uiPipeLine;
	vk::PipelineLayout  m_pipelineLayout;
	vk::DescriptorSet m_descriptorSet;
	vk::DescriptorSetLayout m_descriptorSetLayout;
	const VulkanBrain& m_brain;
	const SwapChain& m_swapChain;
	vk::UniqueSampler m_sampler;
};


