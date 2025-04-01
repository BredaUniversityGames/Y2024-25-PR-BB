#pragma once
#include "quad_draw_info.hpp"
#include "ui_element.hpp"
#include <memory>
#include <vector>

struct InputManagers;
class UIInputContext;
class UIElement;

class Viewport
{
public:
    Viewport(const glm::uvec2& extend, const glm::uvec2& offset = glm::uvec2(0))
        : _extend(extend)
        , _offset(offset)
    {
    }

    void Update(const InputManagers& inputManagers, UIInputContext& uiInputContext);

    /**
     * adds all the draw data for the ui to the drawList argument, fynction calls SubmitDrawInfo on all the present uiElements in a hierarchical manner.
     * This drawList gets cleared when the uiPipeline records it's commands and thus this function needs to be called before the commandLists are submitted.
     * @param drawList Reference to the vector holding the QuadDrawInfo, which is most likely located in the uiPipeline.
     */
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const;

    template <typename T, typename... Args>
        requires(std::derived_from<T, UIElement> && std::is_constructible_v<T, Args...>)
    std::weak_ptr<T> AddElement(Args&&... args)
    {
        auto newElement = std::make_shared<T>(std::forward<Args>(args)...);
        _baseElements.emplace_back(newElement);

        std::sort(_baseElements.begin(), _baseElements.end(), [&](const std::shared_ptr<UIElement>& v1, const std::shared_ptr<UIElement>& v2)
            { return v1->zLevel < v2->zLevel; });

        return newElement;
    }

    template <typename T>
        requires(std::derived_from<T, UIElement>)
    std::weak_ptr<T> AddElement(std::shared_ptr<T> typePtr)
    {
        _baseElements.emplace_back(typePtr);
        std::sort(_baseElements.begin(), _baseElements.end(), [&](const std::shared_ptr<UIElement>& v1, const std::shared_ptr<UIElement>& v2)
            { return v1->zLevel < v2->zLevel; });

        return typePtr;
    }

    void ClearViewport()
    {
        _clearAtEndOfFrame = true;
    }

    NO_DISCARD std::vector<std::shared_ptr<UIElement>>& GetElements() { return _baseElements; }
    NO_DISCARD const std::vector<std::shared_ptr<UIElement>>& CGetElements() const { return _baseElements; }

    const glm::uvec2& GetExtend() { return _extend; }
    const glm::uvec2& GetOffset() { return _offset; }

    void SetExtend(const glm::uvec2 extend) { _extend = extend; }
    void SetOffset(const glm::uvec2 offset) { _offset = offset; }

    static float GetScaleMultipler() { return _uiScaleMultiplier; }
    static void SetScaleMultiplier(float scaleMultipler) { _uiScaleMultiplier = scaleMultipler; }
    static float CalculateScaleMultiplierFromVerticalResolution(float yres)
    {
        return yres / baseYres;
    }

private:
    static constexpr float baseYres = 1440;
    static float _uiScaleMultiplier;

    glm::uvec2 _extend;
    glm::uvec2 _offset;

    // if the viewport should be cleared after the update loop has ran.
    bool _clearAtEndOfFrame = false;

    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::shared_ptr<UIElement>> _baseElements;
};

