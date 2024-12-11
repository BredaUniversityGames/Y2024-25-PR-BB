#include "ui_image.hpp"

#include <input/input_manager.hpp>

void UIImageElement::Update(InputManager& input)
{
    if ((visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
        && !input.HasInputBeenConsumed())
    {
        glm::ivec2 mousePos;
        input.GetMousePosition(mousePos.x, mousePos.y);

        // mouse inside boundary
        if (mousePos.x > static_cast<uint16_t>(GetAbsoluteLocation().x)
            && mousePos.x < static_cast<uint16_t>(GetAbsoluteLocation().x + GetAbsoluteScale().x)
            && mousePos.y > static_cast<uint16_t>(GetAbsoluteLocation().y)
            && mousePos.y < static_cast<uint16_t>(GetAbsoluteLocation().y + GetAbsoluteScale().y))
        {
            input.SetInputConsumed();
        }
    }
    UIElement::Update(input);
}
void UIImageElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        QuadDrawInfo info {
            .matrix = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsoluteLocation(), 0)), glm::vec3(GetAbsoluteScale(), 0))),
            .textureIndex = _image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        ChildrenSubmitDrawInfo(drawList);
    }
}