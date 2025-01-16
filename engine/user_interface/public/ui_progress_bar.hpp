#pragma once
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "ui_element.hpp"
#include <functional>

class UIProgressBar : public UIElement
{
public:
    struct BarStyle
    {
        ResourceHandle<GPUImage> filled = {};
        ResourceHandle<GPUImage> empty = {};

        enum class FillStyle
        {
            eStretch,
            eMask
        } fillStyle
            = FillStyle::eStretch;

        enum class FillDirection
        {
            eFromCenterHorizontal,
            eFromBottom,
        } fillDirection
            = FillDirection::eFromCenterHorizontal;

    } style {};

    UIProgressBar(BarStyle barStyle)
        : style(std::move(barStyle))
    {
    }
    UIProgressBar(BarStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(std::move(aStyle))

    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void SetPercentage(float percentage);
    float GetPercentage() { return _percentageFilled; }

private:
    float _percentageFilled = 0.5f;
};
