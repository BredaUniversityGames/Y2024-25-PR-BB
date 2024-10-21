#pragma once

#include "glm/vec2.hpp"
#include "spdlog/spdlog.h"

#include <memory>
#include <stdint.h>
#include <glm/vec3.hpp>

class InputManager;
class UIPipeline;
/**
 * Base class from which all ui elements inherit. Updating and rendering of the ui happens
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

    void SetLocation(const glm::vec2& location) { _relativeLocation = location; }

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     */
    void UpdateAbsoluteLocation(const glm::vec2& location, bool updateChildren = true)
    {
        _absoluteLocation = location;
        if (updateChildren)
            UpdateChildAbsoluteLocations();
    }

    /**
     * @return the location of the element relative to the set anchorpoint of the parent element.
     */
    [[nodiscard]] const glm::vec2& GetRelativeLocation() const { return _relativeLocation; }

    virtual void SubmitDrawInfo(UIPipeline& pipeline) const
    {
    }

    virtual void Update([[maybe_unused]] const InputManager& input)
    {
        for (auto& i : _children)
            i->Update(input);
    }

    void AddChild(std::unique_ptr<UIElement> child)
    {
        if (_children.size() < _maxChildren && child != nullptr)
            _children.push_back(std::move(child));
        else
            spdlog::warn("UIElement::AddChild: Can't add, Too many children");
    }

    [[nodiscard]] const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
    {
        return _children;
    }

    AnchorPoint anchorPoint
        = AnchorPoint::eMiddle;

    bool visible = true;

    virtual void UpdateChildAbsoluteLocations() = 0;

    glm::vec2 scale {};

    virtual ~UIElement() = default;

private:
    glm::vec2 _absoluteLocation {};
    glm::vec2 _relativeLocation {};

    uint16_t _maxChildren = 0;
    std::vector<std::unique_ptr<UIElement>> _children {};
};
