#pragma once

#include "common.hpp"
#include "input/action_manager.hpp"
#include "quad_draw_info.hpp"
#include "ui_navigation.hpp"

#include <algorithm>
#include <cstdint>
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

class UIInputContext;
struct InputManagers;

/**
 * Base class from which all ui elements inherit. Updating and submitting of the ui happens
 * mostly in a hierarchical manner. each element calls its children's update and draw functions.
 * class contains pure virtual functions.
 */
class UIElement
{
public:
    UIElement() = default;
    virtual ~UIElement() = default;

    enum class AnchorPoint
    {
        eMiddle,
        eTopLeft,
        eTopRight,
        eBottomLeft,
        eBottomRight,
        eBottomCenter,
        eFill
    } anchorPoint
        = AnchorPoint::eMiddle;
    UINavigationTargets navigationTargets = {};
    int16_t zLevel = 1;

    glm::vec4 display_color = glm::vec4(1, 1, 1, 1);

    void SetLocation(const glm::vec2& location) noexcept;

    // todo: move transform functionality into its own class
    NO_DISCARD const glm::vec2& GetRelativeLocation() const noexcept { return _relativeLocation; }
    NO_DISCARD const glm::vec2& GetAbsoluteLocation() const noexcept { return _absoluteLocation; }

    NO_DISCARD const glm::vec2& GetAbsoluteScale() const noexcept { return _absoluteScale; }
    NO_DISCARD const glm::vec2& GetRelativeScale() const noexcept { return _relativeScale; }

    void SetScale(const glm::vec2& scale) noexcept;

    virtual void SubmitDrawInfo(MAYBE_UNUSED std::vector<QuadDrawInfo>& drawList) const = 0;
    virtual void Update(const InputManagers& inputManagers, UIInputContext& uiInputContext);

    template <typename T, typename... Args>
        requires(std::derived_from<T, UIElement> && std::is_constructible_v<T, Args...>)
    std::shared_ptr<T> AddChild(Args&&... args);

    NO_DISCARD std::vector<std::shared_ptr<UIElement>>& GetChildren()
    {
        return _children;
    }

    enum class VisibilityState
    {
        eUpdatedAndVisible,
        eUpdatedAndInvisble,
        eNotUpdatedAndVisible,
        eNotUpdatedAndInvisible
    } visibility
        = VisibilityState::eUpdatedAndVisible;

    virtual void UpdateAllChildrenAbsoluteTransform();


    void SetPreTransformationMatrix(const glm::mat4& matrix){_preTransformationMatrix = matrix;}
    const glm::mat4& GetPreTransformationMatrix() const {return _preTransformationMatrix;}

    //  note: Mostly for internal use to calculate the correct screen space position based on it's parents.
    //  warning: does not update the local transform!
    void SetAbsoluteTransform(const glm::vec2& location, const glm::vec2& scale, bool updateChildren = true) noexcept;

protected:
    void ChildrenSubmitDrawInfo(MAYBE_UNUSED std::vector<QuadDrawInfo>& drawList) const;

private:
    // Steam deck verification requires 9px minimum, but recommends 12px
    static constexpr float MIN_STEAM_SCALE_RECOMMENDATION = 12.0f;

    glm::vec2 _absoluteLocation {};
    glm::vec2 _relativeLocation {};

    glm::vec2 _relativeScale {};
    glm::vec2 _absoluteScale {};
    
    glm::mat4 _preTransformationMatrix = glm::mat4(1);

    std::vector<std::shared_ptr<UIElement>> _children {};
};

template <typename T, typename... Args>
    requires(std::derived_from<T, UIElement> && std::is_constructible_v<T, Args...>)
std::shared_ptr<T> UIElement::AddChild(Args&&... args)
{
    std::shared_ptr<UIElement> addedChild = _children.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    std::sort(_children.begin(), _children.end(), [&](const std::shared_ptr<UIElement>& v1, const std::shared_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });

    UpdateAllChildrenAbsoluteTransform();

    return std::static_pointer_cast<T>(addedChild);
}
