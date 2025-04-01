#include "ui_button.hpp"
#include "input/input_device_manager.hpp"
#include "ui_input.hpp"
#include "ui_module.hpp"
#include <glm/gtc/matrix_transform.hpp>

void UIButton::SwitchState(bool inputActionPressed, bool inputActionReleased)
{
    switch (state)
    {
    case ButtonState::eNormal:
        state = ButtonState::eHovered;
        [[fallthrough]];

    case ButtonState::eHovered:
        if (inputActionPressed)
        {
            state = ButtonState::ePressed;
        }
        break;
    case ButtonState::ePressed:
        if (inputActionReleased)
        {
            state = ButtonState::eHovered;
        }
        break;
    }
}

void UIButton::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
{
    UIElement::Update(inputManagers, inputContext);
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
    {
        previousState = state;
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
                SwitchState(inputManagers.actionManager.GetDigitalAction(inputContext.GetPressActionName()).IsPressed(), inputManagers.actionManager.GetDigitalAction(inputContext.GetPressActionName()).IsReleased());
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

    if (IsPressedOnce())
    {
        _callback();
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

        glm::mat4 matrix = glm::translate(glm::mat4(1), glm::vec3(GetAbsoluteLocation(), 0));
        matrix = glm::scale(matrix, glm::vec3(GetAbsoluteScale(), 0));

        QuadDrawInfo info {
            .matrix = matrix,
            .textureIndex = image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        ChildrenSubmitDrawInfo(drawList);
    }
}
