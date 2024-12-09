#include "viewport.hpp"
#include "ui_element.hpp"

#include <algorithm>

void Viewport::Update(const InputDeviceManager& input) const
{
    for (const auto& element : _baseElements)
    {
        element->Update(input);
    }
}

void Viewport::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& element : _baseElements)
    {
        element->SubmitDrawInfo(drawList);
    }
}

UIElement& Viewport::AddElement(std::unique_ptr<UIElement> element)
{
    _baseElements.emplace_back(std::move(element));
    std::sort(_baseElements.begin(), _baseElements.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });

    return *_baseElements.back();
}