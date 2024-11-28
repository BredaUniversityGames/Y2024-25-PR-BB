#include "viewport.hpp"
#include "ui_element.hpp"

void Viewport::Update(const InputManager& input) const
{
    for (const auto& i : _baseElements)
    {
        i->Update(input);
    }
}

void Viewport::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& i : _baseElements)
    {
        i->SubmitDrawInfo(drawList);
    }
}

UIElement& Viewport::AddElement(std::unique_ptr<UIElement> element)
{
    _baseElements.emplace_back(std::move(element));
    std::ranges::sort(_baseElements.begin(), _baseElements.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });

    return *_baseElements.back();
}