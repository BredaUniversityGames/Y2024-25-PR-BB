#include "../public/ui_button.hpp"
#include "../../application/public/input_manager.hpp"
#include "pipelines/ui_pipeline.hpp"

#include "glm/glm.hpp"

void UIButton::Update(const InputManager& input)
{
    if (enabled)
    {
        glm::ivec2 mousePos;
        input.GetMousePosition(mousePos.x, mousePos.y);

        // mouse inside boundary
        if (mousePos.x > static_cast<int>(GetAbsouluteLocation().x)
            && mousePos.x < static_cast<int>(GetAbsouluteLocation().x + GetScale().x)
            && mousePos.y > static_cast<int>(GetAbsouluteLocation().y)
            && mousePos.y < static_cast<int>(GetAbsouluteLocation().y + GetScale().y))
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
}

void UIButton::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{

    if (enabled)
    {
        ResourceHandle<GPUImage> image;
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
            .modelMatrix = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsouluteLocation(), 0)), glm::vec3(GetScale(), 0))),
            .textureIndex = image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);

        for (auto& i : GetChildren())
        {
            i->SubmitDrawInfo(drawList);
        }
    }
}

void UIButton::UpdateAllChildrenAbsoluteLocations()
{
    for (auto& i : GetChildren())
        i->SetAbsoluteLocation(this->GetAbsouluteLocation() + (GetScale() / 2.f) + i->GetRelativeLocation());
}
