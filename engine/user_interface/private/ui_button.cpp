#include "ui_button.hpp"
#include "glm/gtx/transform.hpp"
#include "pipelines/ui_pipeline.hpp"

#include "input/input_device_manager.hpp"

void UIButton::Update(const ActionManager& input)
{
    if ((visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
        && !input.HasInputBeenConsumed())
    {

        glm::ivec2 mousePos;
        input.GetMousePosition(mousePos.x, mousePos.y);

        // mouse inside boundary
        if ((mousePos.x > static_cast<uint16_t>(GetAbsoluteLocation().x)
                && mousePos.x < static_cast<uint16_t>(GetAbsoluteLocation().x + GetScale().x)
                && mousePos.y > static_cast<uint16_t>(GetAbsoluteLocation().y)
                && mousePos.y < static_cast<uint16_t>(GetAbsoluteLocation().y + GetScale().y))
            || GetNavigation().CurrentlyHasKeyFocus())
        {
            switch (state)
            {
            case ButtonState::eNormal:
                state = ButtonState::eHovered;
                onBeginHoverCallBack();
                input.SetInputConsumed();
                [[fallthrough]];

            case ButtonState::eHovered:
                if (input.GetDigitalAction("UIPress"))
                {
                    state = ButtonState::ePressed;
                    input.SetInputConsumed();
                    onMouseDownCallBack();
                }
                break;

            case ButtonState::ePressed:
                if (!input.GetDigitalAction("UIPress"))
                {
                    state = ButtonState::eNormal;
                }
                break;
            }
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

        QuadDrawInfo info {
            .matrix = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsoluteLocation(), 0)), glm::vec3(GetAbsoluteScale(), 0))),
            .textureIndex = image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        UIElement::ChildrenSubmitDrawInfo(drawList);
    }
}

void UIButton::UpdateAllChildrenAbsoluteTransform()
{
    for (const auto& child : GetChildren())
    {
        child->SetAbsoluteTransform(this->GetAbsoluteLocation() + (GetAbsoluteScale() / 2.f) + child->GetRelativeLocation(), child->GetRelativeScale());
    }
}
