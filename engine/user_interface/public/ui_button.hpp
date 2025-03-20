#pragma once
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_element.hpp"
#include <functional>

class UIButton : public UIElement
{
public:
    UIButton()
    {
    }

    enum class ButtonState
    {
        eNormal,
        eHovered,
        ePressed
    } state
        = ButtonState::eNormal;
    ButtonState previousState = ButtonState::eNormal;

    void Update(const InputManagers& inputManagers, UIInputContext& inputContext) override;

    ButtonState GetState() const { return state; }
    bool IsPressedOnce() const { return state == ButtonState::ePressed && previousState != ButtonState::ePressed; }

    struct ButtonStyle
    {
        ResourceHandle<GPUImage> normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    UIButton(ButtonStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(aStyle)
    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

private:
    void SwitchState(bool inputActionPressed, bool inputActionReleased);
};
