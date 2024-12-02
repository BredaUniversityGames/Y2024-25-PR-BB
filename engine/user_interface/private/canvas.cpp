#include "canvas.hpp"
#include "pipelines/ui_pipeline.hpp"

void Canvas::UpdateAllChildrenAbsoluteLocations()
{
    if (enabled)
    {
        for (const auto& child : GetChildren())
        {
            glm::vec2 childRelativeLocation = child->GetRelativeLocation();

            glm::vec2 newChildLocation;
            switch (child->anchorPoint)
            {
            case AnchorPoint::eMiddle:
                newChildLocation = GetAbsouluteLocation() + (GetScale() / 2.0f) + childRelativeLocation;
                break;
            case AnchorPoint::eTopLeft:
                newChildLocation = { GetAbsouluteLocation() + childRelativeLocation };
                break;
            case AnchorPoint::eTopRight:
                newChildLocation = { GetAbsouluteLocation().x + GetScale().x - childRelativeLocation.x, GetAbsouluteLocation().y + childRelativeLocation.y };
                break;
            case AnchorPoint::eBottomLeft:
                newChildLocation = { GetAbsouluteLocation().x + childRelativeLocation.x, GetAbsouluteLocation().y + GetScale().y - childRelativeLocation.y };
                break;
            case AnchorPoint::eBottomRight:

                break;
            }

            child->SetAbsoluteLocation(newChildLocation);
            child->UpdateAllChildrenAbsoluteLocations();
        }
    }
}

void Canvas::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    for (const auto& child : GetChildren())
    {
        child->SubmitDrawInfo(drawList);
    }
}
