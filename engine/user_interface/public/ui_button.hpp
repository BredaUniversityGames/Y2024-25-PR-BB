//
// Created by luuk on 30-9-2024.
//
#pragma once
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_core.hpp"
#include <functional>

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

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void UpdateChildAbsoluteLocations() override;

    struct ButtonStyle
    {
        ResourceHandle<GPUImage>
            normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    std::function<void()> onBeginHoverCallBack {};
    std::function<void()> onMouseDownCallBack {};

private:
};
