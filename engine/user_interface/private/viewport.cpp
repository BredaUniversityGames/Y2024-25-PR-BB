#include "viewport.hpp"
#include "ui_element.hpp"
#include <ranges>
#include <algorithm>

void Viewport::Update(const ActionManager& input) const
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
