#pragma once
#include <glm/glm.hpp>

struct TransformComponent
{
private:
    glm::vec3 _localPosition {};
    glm::quat _localRotation = { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 _localScale = { 1.0f, 1.0f, 1.0f };
    friend class TransformHelpers;
    friend class Editor;
};