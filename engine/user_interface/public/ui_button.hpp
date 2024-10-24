//
// Created by luuk on 30-9-2024.
//
#pragma once
#include <functional>

#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_core.hpp"

struct UIButton : public UIElement
{
public:
    UIButton()
        : UIElement(1)
    {
    }

    enum class ButtonState
    {
        eNormal,
        eHovered,
        ePressed
    } m_State = ButtonState::eNormal;

    void Update(const InputManager& inputManager) override;
 
    void SubmitDrawInfo(UIPipeline& uiPipeline) const override;

    void UpdateChildAbsoluteLocations() override;

    struct ButtonStyle
    {
        ResourceHandle<Image> normalImage = {};
        ResourceHandle<Image> hoveredImage = {};
        ResourceHandle<Image> pressedImage = {};
    } style {};
    
    std::function<void()> onBeginHoverCallBack {};
    std::function<void()> oOnMouseDownCallBack {};
};

class UIButtonRenderSystem : public UIRenderSystem<ButtonDrawInfo>
{
public:
    explicit UIButtonRenderSystem(std::shared_ptr<UIPipeLine>& pl)
        : UIRenderSystem<ButtonDrawInfo>(pl)
    {
    }

    void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain&) override;

    ~UIButtonRenderSystem() override = default;
};
