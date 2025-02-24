#include "components/directional_light_component.hpp"

#include "glm/gtc/type_ptr.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<DirectionalLightComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<DirectionalLightComponent>(e);
    t.Inspect();
}
}
void DirectionalLightComponent::Inspect()
{
    ImGui::ColorPicker3("Color##Direcional Light", &color.x);

    ImGui::Text("Shadow Settings##Direcional Light");
    ImGui::DragFloat("Shadow Bias##Direcional Light", &shadowBias, 0.01f);
    ImGui::DragFloat("Poisson World Offset##Direcional Light", &poissonWorldOffset, 1.0f);
    ImGui::DragFloat("Poisson Constant##Direcional Light", &poissonConstant, 1.0f);
    ImGui::DragFloat("Orthographic Size##Direcional Light", &orthographicSize, 0.1f);
    ImGui::DragFloat("Near Plane##Direcional Light", &nearPlane, 0.1f);
    ImGui::DragFloat("Far Plane##Direcional Light", &farPlane, 0.1f);
    ImGui::DragFloat("Aspect Ratio##Direcional Light", &aspectRatio, 0.1f);
}