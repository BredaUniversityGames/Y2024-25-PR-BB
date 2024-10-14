#pragma once

struct WorldMatrixComponent
{
private:
    glm::mat4 _worldMatrix { 1.0f };
    friend class TransformHelpers;
};