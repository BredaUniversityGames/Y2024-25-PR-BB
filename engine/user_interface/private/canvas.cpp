#include "canvas.hpp"
#include "pipelines/ui_pipeline.hpp"

#include <iostream>

void Canvas::UpdateAllChildrenAbsoluteLocations()
{
    {
        std::cout << "test" << std::endl;
        for (const auto& i : GetChildren())
        {
            auto relativeLocation = i->GetRelativeLocation();
            switch (i->anchorPoint)
            {
            case AnchorPoint::eMiddle:
                i->SetAbsoluteLocation(GetAbsouluteLocation() + (scale / 2.0f) + relativeLocation);
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
