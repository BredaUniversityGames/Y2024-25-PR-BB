#include "ui_slider.hpp"
#include "input/input_device_manager.hpp"
#include "ui_input.hpp"
#include <glm/gtc/matrix_transform.hpp>

void UISlider::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
{
    UIElement::Update(inputManagers, inputContext);

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

        glm::vec2 movement = inputManagers.actionManager.GetAnalogAction("Navigate");

        if (std::abs(movement.x) >= inputContext._inputDeadZone)
        {
            constexpr float SLIDER_SPEED = 0.001f;

            value += movement.x * SLIDER_SPEED * inputContext.deltatime.count();
            value = std::clamp(value, 0.0f, 1.0f);

            if (_callback)
                _callback(value);
        }
    }
    else // Mouse controls
    {
        glm::ivec2 mousePos;
        inputManagers.inputDeviceManager.GetMousePosition(mousePos.x, mousePos.y);

        bool mouseIn = IsMouseInsideBoundary(mousePos, GetAbsoluteLocation(), GetAbsoluteScale());
        bool mouseDown = inputManagers.inputDeviceManager.IsMouseButtonPressed(MouseButton::eBUTTON_LEFT);
        bool mouseUp = inputManagers.inputDeviceManager.IsMouseButtonReleased(MouseButton::eBUTTON_LEFT);

        if (mouseIn && mouseDown)
        {
            selected = true;
        }
        else if (mouseUp)
        {
            selected = false;
        }

        if (selected)
        {
            float min = GetAbsoluteLocation().x;
            float max = min + GetAbsoluteScale().x;

            value = glm::clamp((mousePos.x - min) / (max - min), 0.0f, 1.0f);

            if (_callback)
                _callback(value);

            inputContext.ConsumeInput();
        }
    }
}

void UISlider::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        constexpr glm::vec4 SELECTED = { 0.7f, 0.7f, 0.7f, 1.0f };
        constexpr glm::vec4 NORMAL = { 1.0f, 1.0f, 1.0f, 1.0f };

        auto makeMat = [](glm::vec2 translation, glm::vec2 scale)
        {
            return glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(translation, 0.0f)), glm::vec3(scale, 0.0f));
        };

        QuadDrawInfo info {
            .matrix = makeMat(GetAbsoluteLocation(), GetAbsoluteScale()),
            .color = selected ? SELECTED : NORMAL,
            .textureIndex = style.empty.Index()
        };

        glm::vec3 fullScale = { GetAbsoluteScale().x * value, GetAbsoluteScale().y, 0.0f };

        QuadDrawInfo fullInfo {
            .matrix = makeMat(GetAbsoluteLocation(), fullScale),
            .color = selected ? SELECTED : NORMAL,
            .textureIndex = style.filled.Index()
        };

        glm::vec2 knobPos = GetAbsoluteLocation();
        knobPos.x += fullScale.x - style.knobSize.x * 0.5f;
        knobPos.y += GetAbsoluteScale().y * 0.5f - style.knobSize.y * 0.5f;

        QuadDrawInfo knobInfo {
            .matrix = makeMat(knobPos, style.knobSize),
            .color = selected ? SELECTED : NORMAL,
            .textureIndex = style.knob.Index()
        };

        info.useRedAsAlpha = false;
        fullInfo.useRedAsAlpha = false;
        knobInfo.useRedAsAlpha = false;

        drawList.emplace_back(info);
        drawList.emplace_back(fullInfo);
        drawList.emplace_back(knobInfo);

        ChildrenSubmitDrawInfo(drawList);
    }
}
