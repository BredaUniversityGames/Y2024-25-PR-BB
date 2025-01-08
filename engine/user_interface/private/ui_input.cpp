#include "ui_input.hpp"
UINavigationDirection UIInputContext::GetDirection(const ActionManager& actionManager)
{
    glm::vec2 actionValue = actionManager.GetAnalogAction(_navigationActionName);
    glm::vec2 absActionValue = glm::abs(actionValue);

    if (glm::length(actionValue) < _inputDeadZone)
    {
        _previousNavigationDirection = UINavigationDirection::eNone;
        return UINavigationDirection::eNone;
    }

    UINavigationDirection direction = UINavigationDirection::eNone;

    if (absActionValue.x > absActionValue.y)
    {
        if (actionValue.x > _inputDeadZone)
        {
            direction = UINavigationDirection::eRight;
        }
        else if (actionValue.x < -_inputDeadZone)
        {
            direction = UINavigationDirection::eLeft;
        }
    }
    else
    {
        if (actionValue.y > _inputDeadZone)
        {
            direction = UINavigationDirection::eUp;
        }
        else if (actionValue.y < -_inputDeadZone)
        {
            direction = UINavigationDirection::eDown;
        }
    }

    // Return None if direction hasn't changed
    if (direction == _previousNavigationDirection)
    {
        return UINavigationDirection::eNone;
    }

    // Cache the direction to compare against the next time this function gets called.
    _previousNavigationDirection = direction;
    return direction;
}