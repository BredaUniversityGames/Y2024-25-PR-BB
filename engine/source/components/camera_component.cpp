#include "components/camera_component.hpp"
#include "log.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace EnttEditor
{
template <>
void ComponentEditorWidget<CameraComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<CameraComponent>(e);
    t.Inspect();
}
}
void CameraComponent::Inspect()
{
    int32_t currentProjection = static_cast<int32_t>(projection);
    constexpr int32_t projectionElementCount = static_cast<int32_t>(Projection::eCount);
    const char* projectionElementNames[projectionElementCount] = { "Perspective", "Orthographic" };
    const char* currentProjectionName = "Unknown";

    if (currentProjection >= 0 && currentProjection < projectionElementCount)
    {
        currentProjectionName = projectionElementNames[currentProjection];
    }
    else
    {
        bblog::warn("CameraComponent", "Invalid projection type: {}", currentProjection);
    }

    ImGui::SliderInt("Projection##Camera", &currentProjection, 0, projectionElementCount - 1, currentProjectionName);

    ImGui::DragFloat("Field of View##Camera", &fov, 1.0f);
    ImGui::DragFloat("Near Plane##Camera", &nearPlane, 0.1f);
    ImGui::DragFloat("Far Plane##Camera", &farPlane, 0.1f);
}