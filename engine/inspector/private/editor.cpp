#include "editor.hpp"
#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "bloom_settings.hpp"
#include "cheats_component.hpp"
#include "components/animation_transform_component.hpp"
#include "components/camera_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/relationship_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "emitter_component.hpp"
#include "imgui_backend.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "systems/lifetime_component.hpp"
#include "systems/physics_system.hpp"

#include <entt/entity/entity.hpp>
#include <misc/cpp/imgui_stdlib.h>

Editor::Editor(ECSModule& ecs)
    : _ecs(ecs)
    , _entityEditor(ecs)
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    _entityEditor.registerComponent<TransformComponent>("Transform");
    _entityEditor.registerComponent<NameComponent>("Name");
    _entityEditor.registerComponent<RelationshipComponent>("Relationship");
    _entityEditor.registerComponent<WorldMatrixComponent>("World Matrix");
    _entityEditor.registerComponent<PointLightComponent>("Point Light");
    _entityEditor.registerComponent<DirectionalLightComponent>("Directional Light");
    _entityEditor.registerComponent<CameraComponent>("Camera");
    _entityEditor.registerComponent<AudioEmitterComponent>("Audio Emitter");
    _entityEditor.registerComponent<AudioListenerComponent>("Audio Listener");
    _entityEditor.registerComponent<ParticleEmitterComponent>("Particle Emitter");
    _entityEditor.registerComponent<LifetimeComponent>("Lifetime");
    _entityEditor.registerComponent<CheatsComponent>("Cheats");
    _entityEditor.registerComponent<AnimationControlComponent>("Animation Control");
    _entityEditor.registerComponent<AnimationChannelComponent>("Animation Channel");
    _entityEditor.registerComponent<AnimationTransformComponent>("Animation Transform");
}

void Editor::DrawHierarchy()
{
    ZoneNamedN(displayHierarchy, "World DisplayHierarchy", true);
    const auto displayEntity = [&](const auto& self, entt::entity entity) -> void
    {
        RelationshipComponent* relationship = _ecs.GetRegistry().try_get<RelationshipComponent>(entity);
        const char* name = NameComponent::GetDisplayName(_ecs.GetRegistry(), entity).data();
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (relationship != nullptr && relationship->childrenCount > 0)
        {
            const bool nodeOpen = ImGui::TreeNodeEx(std::bit_cast<void*>(static_cast<size_t>(entity)), nodeFlags, "%s", name);

            if (ImGui::IsItemClicked())
            {
                _selectedEntity = entity;
            }

            if (nodeOpen)
            {
                entt::entity current = relationship->first;
                for (size_t i = 0; i < relationship->childrenCount; ++i)
                {
                    if (_ecs.GetRegistry().valid(current))
                    {
                        self(self, current);
                        current = _ecs.GetRegistry().get<RelationshipComponent>(current).next;
                    }
                }

                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(std::bit_cast<void*>(static_cast<size_t>(entity)), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", name);
            if (ImGui::IsItemClicked())
            {
                _selectedEntity = entity;
            }
        }
    };

    if (ImGui::Begin("World Inspector"))
    {
        if (ImGui::Button("+ Add entity"))
        {
            entt::entity entity = _ecs.GetRegistry().create();

            _ecs.GetRegistry().emplace<TransformComponent>(entity);
        }

        if (ImGui::BeginChild("Hierarchy Panel"))
        {
            for (const auto [entity] : _ecs.GetRegistry().storage<entt::entity>().each())
            {
                RelationshipComponent* relationship = _ecs.GetRegistry().try_get<RelationshipComponent>(entity);
                bool shouldHideOrphan = _ecs.GetRegistry().any_of<HideOrphan>(entity);

                if (!shouldHideOrphan && (relationship == nullptr || relationship->parent == entt::null))
                {
                    displayEntity(displayEntity, entity);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void Editor::DrawEntityEditor()
{
    ZoneNamedN(entityEditor, "Entity Editor", true);
    _entityEditor.renderSimpleCombo(_ecs.GetRegistry(), _selectedEntity);

    if (ImGui::Begin("Entity Details"))
    {
        DisplaySelectedEntityDetails();
    }
    ImGui::End();
}

void Editor::DisplaySelectedEntityDetails()
{
    if (_selectedEntity == entt::null)
    {
        ImGui::Text("No entity selected");
        return;
    }

    if (!_ecs.GetRegistry().valid(_selectedEntity))
    {
        ImGui::Text("Selected entity is not valid");
        return;
    }
    const std::string name = std::string(NameComponent::GetDisplayName(_ecs.GetRegistry(), _selectedEntity));
    ImGui::LabelText("##EntityDetails", "%s", name.c_str());

    if (ImGui::Button("Delete"))
    {
        _ecs.DestroyEntity(_selectedEntity);
        _selectedEntity = entt::null;
        return;
    }
    ImGui::PushID(static_cast<int>(_selectedEntity));

    TransformComponent* transform = _ecs.GetRegistry().try_get<TransformComponent>(_selectedEntity);
    NameComponent* nameComponent = _ecs.GetRegistry().try_get<NameComponent>(_selectedEntity);
    if (transform != nullptr)
    {
        bool changed = false;
        // Inspect Transform component
        // TODO use euler angles instead of quaternion
        changed |= ImGui::DragFloat3("Position", &transform->_localPosition.x);
        changed |= ImGui::DragFloat4("Rotation", &transform->_localRotation.w);
        changed |= ImGui::DragFloat3("Scale", &transform->_localScale.x);

        if (changed)
        {
            TransformHelpers::UpdateWorldMatrix(_ecs.GetRegistry(), _selectedEntity);
        }
    }

    RigidbodyComponent* rigidbody = _ecs.GetRegistry().try_get<RigidbodyComponent>(_selectedEntity);
    if (rigidbody != nullptr)
    {
        _ecs.GetSystem<PhysicsSystem>()->InspectRigidBody(*rigidbody);
    }

    if (nameComponent != nullptr)
    {
        ImGui::InputText("Name", &nameComponent->name);
    }

    ImGui::PopID();
    // inspect other components
}

Editor::~Editor() = default;
