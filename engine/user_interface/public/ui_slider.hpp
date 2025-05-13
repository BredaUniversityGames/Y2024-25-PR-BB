#pragma once
#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "ui_element.hpp"

#include <functional>

class UISlider : public UIElement
{
public:
    UISlider() = default;

    struct SliderStyle
    {
        ResourceHandle<GPUImage> empty = {};
        ResourceHandle<GPUImage> filled = {};
        ResourceHandle<GPUImage> knob = {};
        glm::vec2 knobSize = { 50.0f, 50.0f };
    } style {};

    UISlider(SliderStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(aStyle)
    {
        SetLocation(location);
        SetScale(size);
    }

    void Update(const InputManagers& inputManagers, UIInputContext& inputContext) override;

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
    void OnSlide(std::function<void(float)> callback) { _callback = callback; }

    float value = 0.5f;

private:
    bool selected {};
    std::function<void(float)> _callback {};
};