#pragma once

class ActionManager;
class UIElement;

class UINavigationMappings
{
public:
    struct ElementMap
    {
        std::optional<std::weak_ptr<UIElement>> up = std::nullopt;
        std::optional<std::weak_ptr<UIElement>> down = std::nullopt;
        std::optional<std::weak_ptr<UIElement>> left = std::nullopt;
        std::optional<std::weak_ptr<UIElement>> right = std::nullopt;
    };

    enum class Direction : uint8_t
    {
        eNone,
        eUp,
        eDown,
        eLeft,
        eRight
    };

    UINavigationMappings(ElementMap elements)
        : _elementMap(std::move(elements))
    {
    }

    UINavigationMappings(ElementMap elements, std::string_view _analogNavigationActionName)
        : _analogNavigationActionName(_analogNavigationActionName)
        , _elementMap(std::move(elements))
    {
    }

    NO_DISCARD Direction GetDirection(const ActionManager& actionManager) const noexcept;
    NO_DISCARD std::optional<std::weak_ptr<UIElement>> GetNavigationElement(Direction direction);

    bool CurrentlyHasKeyFocus() const noexcept
    {
        return _currenthasKeyFocus;
    }

private:
    friend class UIInputHandler;

    bool _currenthasKeyFocus = false;

    ElementMap _elementMap;
    std::string _analogNavigationActionName = "defaultUINavigation";
};
