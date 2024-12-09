#pragma once
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_element.hpp"
#include <functional>

class UIButton : public UIElement
{
public:
    enum class ButtonState
    {
        eNormal,
        eHovered,
        ePressed
    } state
        = ButtonState::eNormal;

    struct ButtonStyle
    {
        ResourceHandle<GPUImage> normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    UIButton() = default;
    UIButton(ButtonStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(aStyle)
    {
        SetLocation(location);
        SetScale(size);
    }

    void Update(InputManager& inputManager) override;

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void UpdateAllChildrenAbsoluteTransform() override;

    std::function<void()> onBeginHoverCallBack = []() {};
    std::function<void()> onMouseDownCallBack = []() {};
};
