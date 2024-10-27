#include "ui_core.hpp"
#include "../public/ui_pipelines.hpp"

void Canvas::UpdateChildAbsoluteLocations()
{
    {
        for (const auto& i : GetChildren())
        {
            auto relativeLocation = i->GetRelativeLocation();
            switch (i->anchorPoint)
            {
            case AnchorPoint::eMiddle:
                i->SetAbsoluteLocation(GetAbsouluteLocation() + relativeLocation);
                break;
            case AnchorPoint::eTopLeft:
                i->SetAbsoluteLocation(GetAbsouluteLocation() + relativeLocation);
                break;
            case AnchorPoint::eTopRight:
                i->SetAbsoluteLocation({ GetAbsouluteLocation().x + scale.x - relativeLocation.x, GetAbsouluteLocation().y + relativeLocation.y });
                break;
            case AnchorPoint::eBottomLeft:
                i->SetAbsoluteLocation({ GetAbsouluteLocation().x + relativeLocation.x, GetAbsouluteLocation().y + scale.y - relativeLocation.y });
                break;
            case AnchorPoint::eBottomRight:
                i->SetAbsoluteLocation(GetAbsouluteLocation() + scale - relativeLocation);
                break;
            }

            i->UpdateChildAbsoluteLocations();
        }
    }
}

void Canvas::SubmitDrawInfo(UIPipeline& piepline) const
{
    for (const auto& i : GetChildren())
    {
        i->SubmitDrawInfo(piepline);
    }
}
