#include "ui_element.hpp"
#include "ui_input.hpp"

#include <ranges>

void UIElement::Update(const InputManagers& inputManagers, UIInputContext& uiInputContext)
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eUpdatedAndInvisble)
    {
        // update navigation.
        if (uiInputContext.GamepadHasFocus() == true && uiInputContext.HasInputBeenConsumed() == false)
        {
            if (auto locked = uiInputContext.focusedUIElement.lock(); locked.get() == this)
            {
                UINavigationDirection direction = uiInputContext.GetDirection(inputManagers.actionManager);
                std::weak_ptr<UIElement> navTarget = GetUINavigationTarget(navigationTargets, direction);

                if (std::shared_ptr<UIElement> locked = navTarget.lock(); locked != nullptr)
                {
                    uiInputContext.focusedUIElement = locked;
                }
            }
        }

        for (auto& child : _children | std::views::reverse)
        {
            child->Update(inputManagers, uiInputContext);
        }
    }
}

void UIElement::UpdateAllChildrenAbsoluteTransform()

{
    for (const auto& child : _children)
    {
        glm::vec2 childRelativeLocation = child->GetRelativeLocation();
        glm::vec2 newChildLocation;
        glm::vec2 newChildScale = child->GetRelativeScale();

        switch (child->anchorPoint)
        {
        case AnchorPoint::eMiddle:
            newChildLocation = GetAbsoluteLocation() + (GetAbsoluteScale() / 2.0f) + (childRelativeLocation - child->_relativeScale / 2.0f);
            break;
        case AnchorPoint::eTopLeft:
            newChildLocation = { GetAbsoluteLocation() + childRelativeLocation };
            break;
        case AnchorPoint::eTopRight:
            newChildLocation = { GetAbsoluteLocation().x + GetAbsoluteScale().x - childRelativeLocation.x, GetAbsoluteLocation().y + childRelativeLocation.y };
            break;
        case AnchorPoint::eBottomLeft:
            newChildLocation = { GetAbsoluteLocation().x + childRelativeLocation.x, GetAbsoluteLocation().y + GetAbsoluteScale().y - childRelativeLocation.y };
            break;
        case AnchorPoint::eBottomRight:
            newChildLocation = { GetAbsoluteLocation().x + GetAbsoluteScale().x - childRelativeLocation.x, GetAbsoluteLocation().y + GetAbsoluteScale().y - childRelativeLocation.y };
            break;
        case AnchorPoint::eFill:
            newChildLocation = { GetAbsoluteLocation() };
            newChildScale = { GetAbsoluteScale() };
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
    {
        UpdateAllChildrenAbsoluteTransform();
    }
}
void UIElement::ChildrenSubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& child : _children)
    {
        child->SubmitDrawInfo(drawList);
    }
}