//
// Created by luuk on 16-9-2024.
//

#pragma once
#include "typeindex"
#include <pch.hpp>
#include "input_manager.hpp"
#include "../../external/entt-master/src/entt/entt.hpp"


struct CameraUBO;
struct Camera;
class SwapChain;


struct UITransform
{
	glm::vec2 Translation {};
	glm::vec2 Scale {};
};


struct Text_Element 
{
	
	
	std::string m_text = "placeholder";


	
	int m_fontSize = 30;
	int m_CharacterDistance = 2;
	
};

struct Button
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
	
	std::function<void()> OnBeginHoverCallBack {};
	std::function<void()> OnMouseDownCallBack {};

	ButtonState m_state = ButtonState::normal;
};


class UIPipeLine;

class UISubSystem
{
public:
	virtual void Update(entt::registry&, const InputManager& input) = 0;

	//todo: create better render abstraction so this needs less parameters.
	virtual void Render(const entt::registry&, vk::CommandBuffer commandBuffer, uint32_t currentFrame,
	                    const ResourceHandle<Image>& render_target,  UIPipeLine& pipe_line, const glm::mat4&) = 0;
};

class UIButtonSubSystem : public UISubSystem
{
public:
	void Update(entt::registry&, const InputManager& input) override;
	void Render(const entt::registry&, vk::CommandBuffer commandBuffer, uint32_t currentFrame,
	            const ResourceHandle<Image>& render_target,  UIPipeLine& pipe_line, const glm::mat4&) override;
};

class UIDisplayTextSubSystem : public UISubSystem
{
public:
	UIDisplayTextSubSystem();
	void Update(entt::registry&, const InputManager& input) override {};
	void Render(const entt::registry&, vk::CommandBuffer commandBuffer, uint32_t currentFrame,
				const ResourceHandle<Image>& render_target,  UIPipeLine& pipe_line, const glm::mat4&) override;


	
};



class UserInterfaceSystem {
public:
	void InitializeDefaultSubSystems();
	void Update(const InputManager& input);
	void Render(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const   ResourceHandle<Image>& _hdrTarget,  UIPipeLine&,const glm::mat4&);
	//todo: add children


	entt::registry ui_Registry;
	
	//todo: add variatic function paramters to pass to subsystem constructor.
	template<typename T>
	void AddSubSystem()
	{
		static_assert(std::is_base_of<UISubSystem,T>::value, "Subsystem must be derived from UISubSystem");
		m_ui_sub_systems_.emplace(std::type_index(typeid(T)) , std::make_unique<T>()).second;
	}

	
protected:
	std::unordered_map<std::type_index, std::unique_ptr<UISubSystem>> m_ui_sub_systems_;
};


class UIPipeLine
{
public:

	void CreatePipeLine();
	UIPipeLine(const VulkanBrain& brain, const SwapChain& sc) : m_brain(brain),m_swapChain(sc) {};

	void CreateDescriptorSetLayout();
	void UpdateTexture(ResourceHandle<Image> image,vk::DescriptorSet& set) const;
	void RecordCommands();
	NON_COPYABLE(UIPipeLine);;
	NON_MOVABLE(UIPipeLine)
	~UIPipeLine();

	VkPipeline m_uiPipeLine;
	vk::PipelineLayout  m_pipelineLayout;
	vk::DescriptorSet m_descriptorSet {};
	static vk::DescriptorSetLayout m_descriptorSetLayout;
	const VulkanBrain& m_brain;
	const SwapChain& m_swapChain;
	vk::UniqueSampler m_sampler;
};
