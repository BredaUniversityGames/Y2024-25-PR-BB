#include "ui_element.hpp"

#include "log.hpp"
#include <algorithm>
#include <ranges>

void UIElement::Update(InputManager& input)
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
    {
        for (auto& child : _children | std::views::reverse)
            child->Update(input);
    }
}

void UIElement::UpdateAllChildrenAbsoluteTransform()
{
    for (const auto& child : GetChildren())
    {
        glm::vec2 childRelativeLocation = child->GetRelativeLocation();
        glm::vec2 newChildLocation;
        glm::vec2 newChildScale = child->GetRelativeScale();
        switch (child->anchorPoint)
        {
        case AnchorPoint::eMiddle:
            newChildLocation = GetAbsoluteLocation() + (GetScale() / 2.0f) + (childRelativeLocation - child->_relativeScale / 2.0f);
            break;
        case AnchorPoint::eTopLeft:
            newChildLocation = { GetAbsoluteLocation() + childRelativeLocation };
            break;
        case AnchorPoint::eTopRight:
            newChildLocation = { GetAbsoluteLocation().x + GetScale().x - childRelativeLocation.x, GetAbsoluteLocation().y + childRelativeLocation.y };
            break;
        case AnchorPoint::eBottomLeft:
            newChildLocation = { GetAbsoluteLocation().x + childRelativeLocation.x, GetAbsoluteLocation().y + GetScale().y - childRelativeLocation.y };
            break;
        case AnchorPoint::eBottomRight:
            newChildLocation = { GetAbsoluteLocation().x + GetScale().x - childRelativeLocation.x, GetAbsoluteLocation().y + GetScale().y - childRelativeLocation.y };
            break;
        case AnchorPoint::eFill:
            newChildLocation = { GetAbsoluteLocation() };
            newChildScale = { GetScale() };
            break;
        }

        child->SetAbsoluteTransform(newChildLocation, newChildScale);
    }
}

void UIElement::SetAbsoluteTransform(const glm::vec2& location, const glm::vec2& scale, bool updateChildren) noexcept
{
    _absoluteLocation = location;
    _absoluteScale = scale;
    if (updateChildren)
        UpdateAllChildrenAbsoluteTransform();
}
void UIElement::ChildrenSubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& child : _children)
    {
        child->SubmitDrawInfo(drawList);
    }
}