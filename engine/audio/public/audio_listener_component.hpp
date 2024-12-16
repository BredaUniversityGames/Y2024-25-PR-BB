#pragma once

struct AudioListenerComponent
{
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AudioListenerComponent>(entt::registry& reg, entt::registry::entity_type e);
}