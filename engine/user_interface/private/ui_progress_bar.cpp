#include "ui_progress_bar.hpp"

#include <glm/gtc/matrix_transform.hpp>

void UIProgressBar::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        glm::mat4 matrixFull = GetPreTransformationMatrix();
        glm::vec2 uv0 = glm::vec2(0, 0);
        glm::vec2 uv1 = glm::vec2(1, 1);

        switch (style.fillDirection)
        {
        case BarStyle::FillDirection::eFromCenterHorizontal:
        {
            float localLocation1x = GetAbsoluteLocation().x + (GetAbsoluteScale() * (0.5f - (_fractionFilled * 0.5f))).x;
            float localLocation2x = GetAbsoluteLocation().x + (GetAbsoluteScale() * (0.5f + (_fractionFilled * 0.5f))).x;
            float scaleX = localLocation2x - localLocation1x;
            matrixFull = glm::translate(matrixFull, glm::vec3(localLocation1x, GetAbsoluteLocation().y, 0));
            matrixFull = glm::scale(matrixFull, glm::vec3(scaleX, GetAbsoluteScale().y, 0));

            if (style.fillStyle == BarStyle::FillStyle::eMask)
            {
                uv0 = glm::vec2(0.5 - _fractionFilled * 0.5f, 0);
                uv1 = glm::vec2(0.5f + _fractionFilled * 0.5f, 1);
            }

            break;
        }
        case BarStyle::FillDirection::eFromBottom:
        {

            
            matrixFull = glm::translate(matrixFull, glm::vec3(GetAbsoluteLocation() + glm::vec2(0, GetAbsoluteScale().y * (1 - _fractionFilled)), 0));
            matrixFull = glm::scale(matrixFull, glm::vec3(GetAbsoluteScale().x, GetAbsoluteScale().y * _fractionFilled, 0));

            if (style.fillStyle == BarStyle::FillStyle::eMask)
            {
                uv0 = glm::vec2(0.0f, 1 - _fractionFilled);
            }

            break;
        }
        case BarStyle::FillDirection::eLeftToRight:
        {
            matrixFull = glm::translate(matrixFull, glm::vec3(GetAbsoluteLocation(), 0));
            matrixFull = glm::scale(matrixFull, glm::vec3(GetAbsoluteScale().x * _fractionFilled, GetAbsoluteScale().y, 0));

            if (style.fillStyle == BarStyle::FillStyle::eMask)
            {
                uv1 = glm::vec2(_fractionFilled, 1.0f);
            }

            break;
        }

        default:
            assert(false && "Direction not implemented yet");
            break;
        }

        glm::mat4 matrixEmpty = GetPreTransformationMatrix();
        matrixEmpty = glm::translate(matrixEmpty, glm::vec3(GetAbsoluteLocation(), 0));
        matrixEmpty = glm::scale(matrixEmpty, glm::vec3(GetAbsoluteScale(), 0));

        QuadDrawInfo infoEmpty {
            .matrix = matrixEmpty,
            .textureIndex = style.empty.Index(),
        };

        QuadDrawInfo infoFull {
            .matrix = matrixFull,
            .uvMin = uv0,
            .uvMax = uv1,
            .textureIndex = style.empty.Index(),
        };

        infoEmpty.useRedAsAlpha = false;
        drawList.emplace_back(infoEmpty);

        infoFull.textureIndex = style.filled.Index();
        drawList.emplace_back(infoFull);

        ChildrenSubmitDrawInfo(drawList);
    }
}
void UIProgressBar::SetFractionFilled(float percentage)
{
    _fractionFilled = glm::clamp(0.f, 1.f, percentage);
}