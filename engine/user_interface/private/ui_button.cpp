//
// Created by luuk on 30-9-2024.
//

#include "../public/ui_button.hpp"
#include "../../application/public/input_manager.hpp"
#include "../public/ui_pipelines.hpp"

void UIButton::Update(const InputManager& input)
{
    glm::ivec2 mousePos;
    input.GetMousePosition(mousePos.x, mousePos.y);

    // mouse inside boundary
    if (mousePos.x > static_cast<int>(GetAbsouluteLocation().x)
        && mousePos.x < static_cast<int>(GetAbsouluteLocation().x + scale.x)
        && mousePos.y > static_cast<int>(GetAbsouluteLocation().y)
        && mousePos.y < static_cast<int>(GetAbsouluteLocation().y + scale.y))
    {
        switch (m_State)
        {
        case ButtonState::eNormal:

            m_State = ButtonState::eHovered;
            onBeginHoverCallBack();
            [[fallthrough]];

        case ButtonState::eHovered:

            if (input.IsMouseButtonPressed(MouseButton::eBUTTON_LEFT))
            {
                m_State = ButtonState::ePressed;
                onMouseDownCallBack();
            }
            break;

        case ButtonState::ePressed:
            if (input.IsMouseButtonReleased(MouseButton::eBUTTON_LEFT))
            {
                m_State = ButtonState::eNormal;
            }
            break;
        }
    }

    else
        m_State = ButtonState::eNormal;
}

void UIButton::SubmitDrawInfo(UIPipeline& pipeline) const
{

    if (visible)
    {
        ResourceHandle<Image>
            image;
        switch (m_State)
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
            .projection = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsouluteLocation(), 0)), glm::vec3(scale, 0))),
            .textureIndex = image.index,
        };

        info.useRedAsAlpha = false;
        pipeline._drawlist.emplace_back(info);

        for (auto& i : GetChildren())
        {
            i->SubmitDrawInfo(pipeline);
        }
    }
}

void UIButton::UpdateChildAbsoluteLocations()
{
    for (auto& i : GetChildren())
        i->SetAbsoluteLocation(this->GetAbsouluteLocation() + (scale / 2.f) + i->GetRelativeLocation());
}
