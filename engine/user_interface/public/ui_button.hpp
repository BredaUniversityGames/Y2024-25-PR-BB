//
// Created by luuk on 30-9-2024.
//
#pragma once
#include <functional>
#include "ui_core.hpp"
#include "resource_manager.hpp"
struct Image;
class UIButton : public UIElement
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
    } m_State
        = ButtonState::eNormal;

    void Update(const InputManager& inputManager) override;

    void SubmitDrawInfo(UIPipeline& uiPipeline) const override;

    void UpdateChildAbsoluteLocations() override;

    struct ButtonStyle
    {
        ResourceHandle<Image>
            normalImage = {};
        ResourceHandle<Image> hoveredImage = {};
        ResourceHandle<Image> pressedImage = {};
    } style {};

    std::function<void()> onBeginHoverCallBack {};
    std::function<void()> onMouseDownCallBack {};

private:
};
