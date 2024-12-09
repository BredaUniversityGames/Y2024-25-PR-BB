#include "editor.hpp"

#include "bloom_settings.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "gbuffers.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "imgui_backend.hpp"
#include "log.hpp"
#include "model_loader.hpp"
#include "performance_tracker.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "serialization.hpp"
#include "systems/physics_system.hpp"
#include "vertex.hpp"

#include <entt/entity/entity.hpp>
#include <fstream>
#include <imgui/misc/cpp/imgui_stdlib.h>

// TODO: Editor shouldnt depend on this.
#include "components/directional_light_component.hpp"
#include "vulkan_context.hpp"

#include <vk_mem_alloc.h>

Editor::Editor(ECSModule& ecs, const std::shared_ptr<Renderer>& renderer, const std::shared_ptr<ImGuiBackend>& imguiBackend)
    : _ecs(ecs)
    , _renderer(renderer)
    , _imguiBackend(imguiBackend)
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    _entityEditor.registerComponent<TransformComponent>("Transform");
    _entityEditor.registerComponent<NameComponent>("Name");
    _entityEditor.registerComponent<RelationshipComponent>("Relationship");
    _entityEditor.registerComponent<WorldMatrixComponent>("WorldMatrix");
    _entityEditor.registerComponent<DirectionalLightComponent>("Directional Light");
}

void Editor::Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings)
{
    _imguiBackend->NewFrame();
    ImGui::NewFrame();

    DrawMainMenuBar();
    // Hierarchy panel
    const auto displayEntity = [&](const auto& self, entt::entity entity) -> void
    {
        RelationshipComponent* relationship = _ecs.GetRegistry().try_get<RelationshipComponent>(entity);
        const std::string name = std::string(NameComponent::GetDisplayName(_ecs.GetRegistry(), entity));
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (relationship != nullptr && relationship->childrenCount > 0)
        {
            const bool nodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<int>(entity)), nodeFlags, "%s", name.c_str());

            if (ImGui::IsItemClicked())
            {
                _selectedEntity = entity;
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                ImGui::SetTooltip("Entity %d", static_cast<int>(entity));
            }

            if (nodeOpen)
            {
                entt::entity current = relationship->first;
                for (size_t i {}; i < relationship->childrenCount; ++i)
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
            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<int>(entity)), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", name.c_str());
            if (ImGui::IsItemClicked())
            {
                _selectedEntity = entity;
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                ImGui::SetTooltip("Entity %d", static_cast<int>(entity));
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

                if (relationship == nullptr || relationship->parent == entt::null)
                {
                    displayEntity(displayEntity, entity);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();

    _entityEditor.renderSimpleCombo(_ecs.GetRegistry(), _selectedEntity);

    if (ImGui::Begin("Entity Details"))
    {
        DisplaySelectedEntityDetails();
    }
    ImGui::End();

    performanceTracker.Render();
    bloomSettings.Render();

    // Render systems inspect
    for (const auto& system : _ecs.GetSystems())
    {
        system->Inspect();
    }

    static ImTextureID textureID = _imguiBackend->GetTexture(_renderer->GetGBuffers().Shadow());
    ImGui::Begin("Directional Light Shadow Map View");
    ImGui::Image(textureID, ImVec2(512, 512));
    ImGui::End();

    ImGui::Begin("Dump VMA stats");

    if (ImGui::Button("Dump json"))
    {
        char* statsJson;
        vmaBuildStatsString(_renderer->GetContext()->VulkanContext()->MemoryAllocator(), &statsJson, true);

        const char* outputFilePath = "vma_stats.json";

        std::ofstream file { outputFilePath };
        if (file.is_open())
        {
            file << statsJson;

            file.close();
        }
        else
        {
            bblog::error("Failed writing VMA stats to file!");
        }

        vmaFreeStatsString(_renderer->GetContext()->VulkanContext()->MemoryAllocator(), statsJson);
    }

    ImGui::End();

    ImGui::Begin("Renderer Stats");

    ImGui::LabelText("Draw calls", "%i", _renderer->GetContext()->GetDrawStats().DrawCalls());
    ImGui::LabelText("Triangles", "%i", _renderer->GetContext()->GetDrawStats().IndexCount() / 3);
    ImGui::LabelText("Indirect draw commands", "%i", _renderer->GetContext()->GetDrawStats().IndirectDrawCommands());

    ImGui::End();

    {
        ZoneNamedN(zone, "ImGui Render", true);
        ImGui::Render();
    }
}
void Editor::DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Scene"))
            {
                // Todo: Load saved scene.
            }
            if (ImGui::MenuItem("Save Scene"))
            {
                Serialization::SerialiseToJSON("assets/maps/scene.json", _ecs);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
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
        changed |= ImGui::DragFloat4("Rotation", &transform->_localRotation.x);
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
