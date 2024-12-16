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

    void Update(const InputDeviceManager& inputManager) override;

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void UpdateAllChildrenAbsoluteLocations() override;

    struct ButtonStyle
    {
        ResourceHandle<GPUImage> normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    std::function<void()> onBeginHoverCallBack {};
    std::function<void()> onMouseDownCallBack {};
};
