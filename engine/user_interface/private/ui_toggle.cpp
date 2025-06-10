#include "ui_toggle.hpp"
#include "input/input_device_manager.hpp"
#include "ui_input.hpp"
#include <glm/gtc/matrix_transform.hpp>

void UIToggle::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
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
        return;
    }

    // Gamepad controls
    if (inputContext.GamepadHasFocus())
    {
        if (auto locked = inputContext.focusedUIElement.lock(); locked.get() != this)
        {
            selected = false;
            return;
        }
        else
        {
            selected = true;
        }

        auto pressAction = inputManagers.actionManager.GetDigitalAction(inputContext.GetPressActionName());

        if (pressAction.IsPressed())
        {
            state = !state;
            if (_callback)
                _callback(state);

            inputContext.ConsumeInput();
        }
    }
    else // Mouse controls
    {
        glm::ivec2 mousePos;
        inputManagers.inputDeviceManager.GetMousePosition(mousePos.x, mousePos.y);

        bool mouseIn = IsMouseInsideBoundary(mousePos, GetAbsoluteLocation(), GetAbsoluteScale());
        bool mouseUp = inputManagers.inputDeviceManager.IsMouseButtonReleased(MouseButton::eBUTTON_LEFT);

        if (mouseIn && mouseUp)
        {
            state = !state;
            if (_callback)
                _callback(state);

            inputContext.ConsumeInput();
        }
    }
}

void UIToggle::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        constexpr glm::vec4 SELECTED = { 0.7f, 0.7f, 0.7f, 1.0f };
        constexpr glm::vec4 NORMAL = { 1.0f, 1.0f, 1.0f, 1.0f };

        glm::mat4 matrix = GetPreTransformationMatrix();
        matrix = glm::translate(matrix, glm::vec3(GetAbsoluteLocation(), 0));
        matrix = glm::scale(matrix, glm::vec3(GetAbsoluteScale(), 0));

        QuadDrawInfo info {
            .matrix = matrix,
            .color = selected ? SELECTED : NORMAL,
            .textureIndex = state ? style.filled.Index() : style.empty.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        ChildrenSubmitDrawInfo(drawList);
    }
}
