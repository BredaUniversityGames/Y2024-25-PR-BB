#include "canvas.hpp"
#include "pipelines/ui_pipeline.hpp"

void Canvas::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& child : GetChildren())
    {
        child->SubmitDrawInfo(drawList);
    }
}
