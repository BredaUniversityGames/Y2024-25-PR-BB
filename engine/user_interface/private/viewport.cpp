#include "viewport.hpp"
#include "ui_pipelines.hpp"
#include "ui_element.hpp"

void Viewport::Update(const InputManager& input) const
{
    for (auto& i : baseElements)
    {
        i->Update(input);
    }
}

void Viewport::Render(UIPipeline& pipeline) const
{
    for (const auto& i : baseElements)
    {
        i->SubmitDrawInfo(pipeline);
    }
}
void Viewport::AddElement(std::unique_ptr<UIElement> element)
{
    baseElements.emplace_back(std::move(element));
    std::sort(baseElements.begin(), baseElements.end(), [&](const std::unique_ptr<UIElement>& v1, const std::unique_ptr<UIElement>& v2)
        { return v1->zLevel < v2->zLevel; });
}