//
// Created by luuk on 30-9-2024.
//
#pragma once
#include <functional>

#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui/UserInterfaceSystem.hpp"

struct ButtonDrawInfo
{
    glm::vec2 position;
    glm::vec2 Scale;
    ResourceHandle<Image> Image;
};

class UIButton : public UIElement
{
public:
    UIButton()
        : UIElement(1)
    {
    }

    enum class ButtonState
    {
        NORMAL,
        HOVERED,
        PRESSED
    } m_State
        = ButtonState::NORMAL;

    void Update(const InputManager&) override;

    void SubmitDrawInfo(UserInterfaceRenderContext&) const override;

    void UpdateChildAbsoluteLocations() override;
    ResourceHandle<Image> m_NormalImage = {};
    ResourceHandle<Image> m_HoveredImage = {};
    ResourceHandle<Image> m_PressedImage = {};

    std::function<void()> m_OnBeginHoverCallBack {};
    std::function<void()> m_OnMouseDownCallBack {};
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
