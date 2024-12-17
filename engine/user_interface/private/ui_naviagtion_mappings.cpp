#include "ui_navigation_mappings.hpp"

std::optional<std::weak_ptr<UIElement>> UINavigationMappings::GetNavigationElement(Direction direction)
{
    switch (direction)
    {
    case Direction::eUp:
        return _elementMap.up;
    case Direction::eDown:
        return _elementMap.up;
    case Direction::eLeft:
        return _elementMap.up;
    case Direction::eRight:
        return _elementMap.up;
    case Direction::eNone:
        return std::nullopt;
    }
}

UINavigationMappings::Direction UINavigationMappings::GetDirection(const ActionManager& actionManager) const noexcept
{
    glm::vec2 actionValue = actionManager.GetAnalogAction(_analogNavigationActionName);
    glm::vec2 absActionValue = glm::abs(actionValue);

    if (absActionValue.x > absActionValue.y && actionValue.x > 0.1f)
    {
        return Direction::eRight;
    }
    if (absActionValue.x > absActionValue.y && actionValue.x < -0.1f)
    {
        return Direction::eLeft;
    }
    if (absActionValue.y > absActionValue.x && actionValue.y > 0.1f)
    {
        return Direction::eUp;
    }
    if (absActionValue.y > absActionValue.x && actionValue.y < -0.1f)
    {
        return Direction::eDown;
    }

    return Direction::eNone;
}
