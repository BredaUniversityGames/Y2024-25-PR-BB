#pragma once

#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "ui_element.hpp"

#include <functional>

class UIToggle : public UIElement
{
public:
    UIToggle() = default;

    struct ToggleStyle
    {
        ResourceHandle<GPUImage> empty = {};
        ResourceHandle<GPUImage> filled = {};
    } style {};

    UIToggle(ToggleStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(aStyle)
    {
        SetLocation(location);
        SetScale(size);
    }

    void Update(const InputManagers& inputManagers, UIInputContext& inputContext) override;

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
    void OnToggle(std::function<void(bool)> callback) { _callback = callback; }

    bool state {};

private:
    bool selected {};
    std::function<void(bool)> _callback {};
};