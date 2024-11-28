#include "canvas.hpp"
#include "pipelines/ui_pipeline.hpp"

void Canvas::UpdateAllChildrenAbsoluteLocations()
{
    if (enabled)
    {
        for (const auto& i : GetChildren())
        {
            glm::vec2 ChildRelativeLocation = i->GetRelativeLocation();

            glm::vec2 newChildLocation;
            switch (i->anchorPoint)
            {
            case AnchorPoint::eMiddle:
                newChildLocation = GetAbsouluteLocation() + (GetScale() / 2.0f) + ChildRelativeLocation;
                break;
            case AnchorPoint::eTopLeft:
                newChildLocation = { GetAbsouluteLocation() + ChildRelativeLocation };
                break;
            case AnchorPoint::eTopRight:
                newChildLocation = { GetAbsouluteLocation().x + GetScale().x - ChildRelativeLocation.x, GetAbsouluteLocation().y + ChildRelativeLocation.y };
                break;
            case AnchorPoint::eBottomLeft:
                newChildLocation = { GetAbsouluteLocation().x + ChildRelativeLocation.x, GetAbsouluteLocation().y + GetScale().y - ChildRelativeLocation.y };
                break;
            case AnchorPoint::eBottomRight:

                break;
            }

            i->SetAbsoluteLocation(newChildLocation);
            i->UpdateAllChildrenAbsoluteLocations();
        }
    }
}

void Canvas::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& i : GetChildren())
    {
        i->SubmitDrawInfo(drawList);
    }
}
