#include "canvas.hpp"

void Canvas::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    ChildrenSubmitDrawInfo(drawList);
}
