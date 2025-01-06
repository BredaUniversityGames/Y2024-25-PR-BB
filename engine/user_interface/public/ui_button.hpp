#pragma once
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_element.hpp"
#include <functional>

class UIButton : public UIElement
{
public:
    UIButton(UINavigationMappings::ElementMap elementMap = {})
        : UIElement(std::move(elementMap))
    {
    }

    enum class ButtonState
    {
        eNormal,
        eHovered,
        ePressed
    } state
        = ButtonState::eNormal;

    void Update(const InputManagers& inputManagers, UIInputContext& inputContext) override;

    struct ButtonStyle
    {
        ResourceHandle<GPUImage> normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    UIButton() = default;
    UIButton(ButtonStyle aStyle, const glm::vec2& location, const glm::vec2& size, UINavigationMappings::ElementMap elementMap = {})
        : style(aStyle)
        , UIElement(std::move(elementMap))
    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void UpdateAllChildrenAbsoluteTransform() override;

    std::function<void()> onBeginHoverCallBack = []() {};
    std::function<void()> onMouseDownCallBack = []() {};

private:
    void SwitchState(bool inputActionPressed, bool inputActionReleased);
};
