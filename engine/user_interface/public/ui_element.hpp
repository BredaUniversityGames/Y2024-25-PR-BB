#pragma once

#include "glm/vec2.hpp"
#include "quad_draw_info.hpp"
#include "spdlog/spdlog.h"
#include <memory>
#include <stdint.h>

class InputManager;
class UIPipeline;
/**
 * Base class from which all ui elements inherit. Updating and submitting of the ui happens
 * mostly in a hierarchical manner. each element calls its children's update and draw functions.
 * class contains pure virtual functions.
 */
class UIElement
{
public:
    explicit UIElement(uint16_t maxChildren)
        : _maxChildren(maxChildren)
    {
    }

    enum class AnchorPoint
    {
        eMiddle,
        eTopLeft,
        eTopRight,
        eBottomLeft,
        eBottomRight,
    };

    /**
     * Whenever this gets called the updateChildrenAbsoluteLocations of the parent needs to get called as well!
     * @param location the new location relative to the set anchor point of the parent.
     */
    void SetLocation(const glm::vec2& location) noexcept { _relativeLocation = location; }

    /**
     * @return the location of the element relative to the set anchorpoint of the parent element.
     */
    NO_DISCARD const glm::vec2& GetRelativeLocation() const noexcept { return _relativeLocation; }
    NO_DISCARD const glm::vec2& GetAbsouluteLocation() const noexcept { return _absoluteLocation; }

    NO_DISCARD const glm::vec2& GetScale() const noexcept { return _scale; }
    void SetScale(const glm::vec2& scale) noexcept { _scale = scale; }

    virtual void SubmitDrawInfo(MAYBE_UNUSED std::vector<QuadDrawInfo>& drawList) const = 0;

    virtual void Update(const InputManager& input)
    {
        for (auto& i : _children)
            i->Update(input);
    }

    UIElement* AddChild(std::unique_ptr<UIElement> child)
    {
        if (_children.size() < _maxChildren && child != nullptr)
        {
            _children.emplace_back(std::move(child));
            std::sort(_children.begin(), _children.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
                { return v1->zLevel < v2->zLevel; });

            UpdateAllChildrenAbsoluteLocations();
            return _children.back().get();
        }

        spdlog::warn("UIElement::AddChild: Can't add, Too many children");
        return nullptr;
    }

    NO_DISCARD const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
    {
        return _children;
    }

    AnchorPoint anchorPoint
        = AnchorPoint::eTopLeft;

    bool enabled = true;
    uint16_t zLevel = 0;

    virtual void UpdateAllChildrenAbsoluteLocations() = 0;

    virtual ~UIElement() = default;

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     */
    void SetAbsoluteLocation(const glm::vec2& location, bool updateChildren = true) noexcept
    {
        _absoluteLocation = location;
        if (updateChildren)
            UpdateAllChildrenAbsoluteLocations();
    }

private:
    glm::vec2 _absoluteLocation {};
    glm::vec2 _relativeLocation {};

    glm::vec2 _scale {};

    uint16_t _maxChildren = 0;
    std::vector<std::unique_ptr<UIElement>> _children {};
};
