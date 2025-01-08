#include "ui_button.hpp"
#include "input/input_device_manager.hpp"
#include "ui_input.hpp"
#include "ui_module.hpp"
#include <glm/glm.hpp>

void UIButton::SwitchState(bool inputActionPressed, bool inputActionReleased)
{
    switch (state)
    {
    case ButtonState::eNormal:
        state = ButtonState::eHovered;
        onBeginHoverCallBack();

        [[fallthrough]];

    case ButtonState::eHovered:
        if (inputActionPressed)
        {
            state = ButtonState::ePressed;
            onMouseDownCallBack();
        }
        break;

    case ButtonState::ePressed:
        if (inputActionReleased)
        {
            state = ButtonState::eNormal;
        }
        break;
    }
}

void UIButton::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
{
    UIElement::Update(inputManagers, inputContext);
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
    {
        if (inputContext.HasInputBeenConsumed() == true)
        {
            state = ButtonState::eNormal;
        }
        else
        {
            if (inputContext.GamepadHasFocus())
            {
                if (auto locked = inputContext.focusedUIElement.lock(); locked.get() != this)
                {
                    state = ButtonState::eNormal;
                    return;
                }
                SwitchState(inputManagers.actionManager.GetDigitalAction("Shoot"), !inputManagers.actionManager.GetDigitalAction("Shoot"));
                if (state == ButtonState::ePressed)
                {
                    std::weak_ptr<UIElement> navTarget = GetUINavigationTarget(navigationTargets, UINavigationDirection::eForward);
                    inputContext.focusedUIElement = navTarget.lock() != nullptr ? navTarget : inputContext.focusedUIElement;
                }
                inputContext.ConsumeInput();
            }
            else // Mouse controls
            {
                glm::ivec2 mousePos;
                inputManagers.inputDeviceManager.GetMousePosition(mousePos.x, mousePos.y);
                if (IsMouseInsideBoundary(mousePos, GetAbsoluteLocation(), GetAbsoluteScale()))
                {
                    SwitchState(inputManagers.inputDeviceManager.IsMouseButtonPressed(MouseButton::eBUTTON_LEFT), inputManagers.inputDeviceManager.IsMouseButtonReleased(MouseButton::eBUTTON_LEFT));
                    inputContext.ConsumeInput();
                }
                else
                {
                    state = ButtonState::eNormal;
                }
            }
        }
    }
}

void UIButton::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {

        ResourceHandle<GPUImage> image;
        switch (state)
        {
        case ButtonState::eNormal:
            image = style.normalImage;
            break;

        case ButtonState::eHovered:
            image = style.hoveredImage;
            break;

        case ButtonState::ePressed:
            image = style.pressedImage;
            break;
        }

        QuadDrawInfo info {
            .matrix = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsoluteLocation(), 0)), glm::vec3(GetAbsoluteScale(), 0))),
            .textureIndex = image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        ChildrenSubmitDrawInfo(drawList);
    }
}
