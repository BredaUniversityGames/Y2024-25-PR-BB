#include "components/point_light_component.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<PointLightComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<PointLightComponent>(e);
    t.Inspect();
}
}
void PointLightComponent::Inspect()
{
    ImGui::ColorPicker4("Color##Point Light", &color.x);
}