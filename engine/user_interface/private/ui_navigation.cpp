#include "ui_navigation.hpp"
#include "ui_element.hpp"

std::weak_ptr<UIElement> GetUINavigationTarget(const UINavigationTargets& navigationTargets, UINavigationDirection direction)
{
    switch (direction)
    {
    case UINavigationDirection::eUp:
        return navigationTargets.up;
    case UINavigationDirection::eDown:
        return navigationTargets.down;
    case UINavigationDirection::eLeft:
        return navigationTargets.left;
    case UINavigationDirection::eRight:
        return navigationTargets.right;
    case UINavigationDirection::eForward:
        return navigationTargets.forward;
    case UINavigationDirection::eBackward:
        return navigationTargets.backward;
    case UINavigationDirection::eNone:
        return std::weak_ptr<UIElement>();
    }
}
