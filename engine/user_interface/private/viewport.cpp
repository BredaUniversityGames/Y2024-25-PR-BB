#include "viewport.hpp"
#include "ui_element.hpp"

#include <algorithm>s
#include <ranges>

void Viewport::Update(InputManager& input)
{
    for (const auto& element : _baseElements | std::views::reverse)
    {
        element->Update(input);
    }

    if (_clearAtEndOfFrame)
    {
        _baseElements.clear();
        _clearAtEndOfFrame = false;
    }
}

void Viewport::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& element : _baseElements)
    {
        element->SubmitDrawInfo(drawList);
    }
}
