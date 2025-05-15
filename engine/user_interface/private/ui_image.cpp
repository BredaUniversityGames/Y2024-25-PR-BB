#include "ui_image.hpp"
#include "quad_draw_info.hpp"

#include <glm/gtc/matrix_transform.hpp>

void UIImage::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    if (visibility == VisibilityState::eUpdatedAndVisible || visibility == VisibilityState::eNotUpdatedAndVisible)
    {
        glm::mat4 matrix = glm::translate(glm::mat4(1), glm::vec3(GetAbsoluteLocation(), 0));
        matrix = glm::scale(matrix, glm::vec3(GetAbsoluteScale(), 0));

        QuadDrawInfo info {
            .matrix = matrix,
            .color = display_color,
            .textureIndex = _image.Index(),
        };

        info.useRedAsAlpha = false;
        drawList.emplace_back(info);
        ChildrenSubmitDrawInfo(drawList);
    }
}
