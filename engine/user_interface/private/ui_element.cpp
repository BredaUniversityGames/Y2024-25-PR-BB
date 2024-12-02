#include "ui_element.hpp"
#include "log.hpp"

void UIElement::Update(const InputManager& input)
{
    for (auto& child : _children)
        child->Update(input);
}
UIElement& UIElement::AddChild(std::unique_ptr<UIElement> child)
{

    _children.emplace_back(std::move(child));
    std::sort(_children.begin(), _children.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });

    UpdateAllChildrenAbsoluteLocations();
    return *_children.back();
}
void UIElement::UpdateAllChildrenAbsoluteLocations()
{
    if (enabled)
    {
        for (const auto& child : GetChildren())
        {
            glm::vec2 childRelativeLocation = child->GetRelativeLocation();

            glm::vec2 newChildLocation;
            switch (child->anchorPoint)
            {
            case AnchorPoint::eMiddle:
                newChildLocation = GetAbsoluteLocation() + (GetScale() / 2.0f) + childRelativeLocation;
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

                break;
            }

            child->SetAbsoluteLocation(newChildLocation);
            child->UpdateAllChildrenAbsoluteLocations();
        }
    }
}
void UIElement::SetAbsoluteLocation(const glm::vec2& location, bool updateChildren) noexcept
{
    _absoluteLocation = location;
    if (updateChildren)
        UpdateAllChildrenAbsoluteLocations();
}