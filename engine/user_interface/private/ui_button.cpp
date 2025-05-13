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

    // Early out - Button should not be updated
    if (visibility == VisibilityState::eNotUpdatedAndInvisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        return;
    }

    // Early out = Input had been consumed by another UI element
    if (inputContext.HasInputBeenConsumed())
    {
        state = ButtonState::eNormal;
        return;
    }

    // Gamepad controls
    if (inputContext.GamepadHasFocus())
    {
        if (auto locked = inputContext.focusedUIElement.lock(); locked.get() != this)
        {
            state = ButtonState::eNormal;
            return;
        }
        else
        {
            state = ButtonState::eHovered;
        }

        auto pressAction = inputManagers.actionManager.GetDigitalAction(inputContext.GetPressActionName());

        if (pressAction.IsHeld())
        {
            state = ButtonState::ePressed;
        }

        if (pressAction.IsReleased())
        {
            _callback();
        }
    }
    else // Mouse controls
    {
        glm::ivec2 mousePos;
        inputManagers.inputDeviceManager.GetMousePosition(mousePos.x, mousePos.y);

        bool mouseIn = IsMouseInsideBoundary(mousePos, GetAbsoluteLocation(), GetAbsoluteScale());
        bool mouseDown = inputManagers.inputDeviceManager.IsMouseButtonPressed(MouseButton::eBUTTON_LEFT);
        bool mouseUp = inputManagers.inputDeviceManager.IsMouseButtonReleased(MouseButton::eBUTTON_LEFT);

        if (mouseIn)
        {
            if (state != ButtonState::ePressed)
            {
                state = ButtonState::eHovered;
            }

            if (mouseDown)
            {
                state = ButtonState::ePressed;
            }

            if (mouseUp)
            {
                _callback();
            }

            inputContext.ConsumeInput();
        }
        else
        {
            state = ButtonState::eNormal;
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
