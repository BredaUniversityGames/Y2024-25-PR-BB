#pragma once

#include "common.hpp"
#include "quad_draw_info.hpp"

#include <cstdint>
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <vector>

class InputManager;

/**
 * Base class from which all ui elements inherit. Updating and submitting of the ui happens
 * mostly in a hierarchical manner. each element calls its children's update and draw functions.
 * class contains pure virtual functions.
 */
class UIElement
{
public:
    UIElement() = default;

    enum class AnchorPoint
    {
        eMiddle,
        eTopLeft,
        eTopRight,
        eBottomLeft,
        eBottomRight,
        eFill
    } anchorPoint
        = AnchorPoint::eMiddle;

    /**
     * Whenever this gets called the updateChildrenAbsoluteLocations of the parent needs to get called as well!
     * @param location the new location relative to the set anchor point of the parent.
     */
    void SetLocation(const glm::vec2& location) noexcept { _relativeLocation = location; }

    /**
     * @return the location of the element relative to the set anchorpoint of the parent element.
     */
    NO_DISCARD const glm::vec2& GetRelativeLocation() const noexcept { return _relativeLocation; }
    NO_DISCARD const glm::vec2& GetAbsoluteLocation() const noexcept { return _absoluteLocation; }

    NO_DISCARD const glm::vec2& GetScale() const noexcept { return _absoluteScale; }
    NO_DISCARD const glm::vec2& GetRelativeScale() const noexcept { return _relativeScale; }

    void SetScale(const glm::vec2& scale) noexcept
    {
        _relativeScale = scale;
        _absoluteScale = scale;
    }

    virtual void SubmitDrawInfo(MAYBE_UNUSED std::vector<QuadDrawInfo>& drawList) const = 0;

    virtual void Update(InputManager& input);

    template <typename T, typename... Args>
        requires(std::derived_from<T, UIElement> && std::is_constructible_v<T, Args...>)
    T& AddChild(Args&&... args)
    {
        UIElement& addedChild = *_children.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
        std::sort(_children.begin(), _children.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
            { return v1->zLevel < v2->zLevel; });

        UpdateAllChildrenAbsoluteTransform();
        return static_cast<T&>(addedChild);
    }

    NO_DISCARD const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
    {
        return _children;
    }

    enum class VisibilityState
    {
        eUpdatedAndVisible,
        eUpdatedAndInvisble,
        eNotUpdatedAndVisible,
        eNotUpdatedAndInvisble
    } visibility
        = VisibilityState::eUpdatedAndVisible;

    int16_t zLevel = 1;

    virtual void UpdateAllChildrenAbsoluteTransform();

    virtual ~UIElement() = default;

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     * @param scale
     */
    void SetAbsoluteTransform(const glm::vec2& location, const glm::vec2& scale, bool updateChildren = true) noexcept;

protected:
    void ChildrenSubmitDrawInfo(MAYBE_UNUSED std::vector<QuadDrawInfo>& drawList) const;

private:
    glm::vec2 _absoluteLocation {};
    glm::vec2 _relativeLocation {};

    glm::vec2 _relativeScale {};
    glm::vec2 _absoluteScale {};

    std::vector<std::unique_ptr<UIElement>> _children {};
};
