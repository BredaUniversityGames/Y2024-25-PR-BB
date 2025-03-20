#include "viewport.hpp"
#include "ui_element.hpp"
#include <algorithm>
#include <ranges>

float Viewport::_uiScaleMultiplier = 0.5f;
void Viewport::Update(const InputManagers& inputManagers, UIInputContext& inputContext)
{
    for (int32_t i = _baseElements.size() - 1; i >= 0; --i)
    {
        _baseElements[i]->Update(inputManagers, inputContext);
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
