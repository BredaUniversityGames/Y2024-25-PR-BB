#include "ui_input.hpp"
UINavigationDirection UIInputContext::GetDirection(const ActionManager& actionManager)
{
    glm::vec2 actionValue = actionManager.GetAnalogAction(_navigationActionName);
    glm::vec2 absActionValue = glm::abs(actionValue);

    UINavigationDirection direction = UINavigationDirection::eNone;
    if (absActionValue.x > absActionValue.y && actionValue.x > 0.1f)
    {
        direction = UINavigationDirection::eRight;
    }
    else if (absActionValue.x > absActionValue.y && actionValue.x < -0.1f)
    {
        direction = UINavigationDirection::eLeft;
    }
    else if (absActionValue.y > absActionValue.x && actionValue.y > 0.1f)
    {
        direction = UINavigationDirection::eUp;
    }
    else if (absActionValue.y > absActionValue.x && actionValue.y < -0.1f)
    {
        direction = UINavigationDirection::eDown;
    }

    if (direction == _previousNavigationDirection)
    {
        return UINavigationDirection::eNone;
    }

    // Cached and checked against current direction to make sure that the continuous
    // analog input is turned into a discrete input until the input is reset.
    _previousNavigationDirection = direction;
    return direction;
}