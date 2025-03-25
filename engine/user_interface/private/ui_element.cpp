#include "ui_element.hpp"
#include "ui_input.hpp"
#include "viewport.hpp"
#include <ranges>


void UIElement::SetLocation(const glm::vec2& location) noexcept
{
    _relativeLocation = location * Viewport::GetScaleMultipler();
}
void UIElement::SetScale(const glm::vec2& scale) noexcept
{
    _relativeScale = scale * Viewport::GetScaleMultipler();
}
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

        for (int32_t i = _children.size() - 1; i >= 0; --i)
        {
            _children[i]->Update(inputManagers, uiInputContext);
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
            newChildLocation = GetAbsoluteLocation() + (GetAbsoluteScale() / 2.0f)
                + (childRelativeLocation - (child->_relativeScale / 2.0f));
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
        case AnchorPoint::eBottomCenter:
        {
            float x = GetAbsoluteLocation().x + (GetAbsoluteScale().x / 2.0f) + (childRelativeLocation.x - child->_relativeScale.x / 2.0f);
            float y = GetAbsoluteLocation().y + GetAbsoluteScale().y - childRelativeLocation.y;
            newChildLocation = glm::vec2(x, y);
            break;
        }
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

    // We make sure it doesn't go under Steam's recommended minimal scale
    const float clampedY = glm::max(scale.y, MIN_STEAM_SCALE_RECOMMENDATION);
    _absoluteScale = glm::vec2(scale.x, clampedY);

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
