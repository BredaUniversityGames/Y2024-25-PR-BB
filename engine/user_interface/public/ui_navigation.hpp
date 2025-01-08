#pragma once

class ActionManager;
class UIElement;

struct UINavigationTargets
{
    std::weak_ptr<UIElement> up;
    std::weak_ptr<UIElement> down;
    std::weak_ptr<UIElement> left;
    std::weak_ptr<UIElement> right;
    std::weak_ptr<UIElement> forward;
    std::weak_ptr<UIElement> backward;
};

enum class UINavigationDirection : uint8_t
{
    eNone,
    eUp,
    eDown,
    eLeft,
    eRight,
    eForward,
    eBackward
};

NO_DISCARD std::weak_ptr<UIElement> GetUINavigationElement(const UINavigationTargets& navigationTargets, UINavigationDirection direction);
