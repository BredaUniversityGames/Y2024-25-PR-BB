#include "ui_element.hpp"

#include "log.hpp"

#include <algorithm>

void UIElement::Update(const InputManager& input)
{
    for (auto& child : _children)
        child->Update(input);
}
UIElement& UIElement::AddChild(std::unique_ptr<UIElement> child)
{
    _children.emplace_back(std::move(child));
    UIElement& addedChild = *_children.back();
    std::sort(_children.begin(), _children.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });

    UpdateAllChildrenAbsoluteTransform();
    return addedChild;
}

void UIElement::UpdateAllChildrenAbsoluteTransform()
{
    if (enabled)
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