#include "viewport.hpp"
#include "ui_element.hpp"
#include <algorithm>
#include <ranges>

void Viewport::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
{
    for (auto& element : _baseElements | std::views::reverse)
    {
        element->Update(inputManagers, inputContext);
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
