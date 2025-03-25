#include "ui_image.hpp"

#include "quad_draw_info.hpp"
void UIImage::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
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
