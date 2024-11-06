#include <imgui_impl_sdl3.h>
#include "editor.hpp"

#include "imgui_impl_vulkan.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui_entt_entity_editor/imgui_entt_entity_editor.hpp"
#include "performance_tracker.hpp"
#include "bloom_settings.hpp"
#include "mesh.hpp"
#include "modules/physics_module.hpp"
#include "profile_macros.hpp"
#include "log.hpp"

#include <fstream>
#include "ECS.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#include "gbuffers.hpp"
#include "renderer.hpp"
#include "serialization.hpp"
#include "ECS.hpp"
#include "model_loader.hpp"
#include "timers.hpp"

#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"

#include <entt/entity/entity.hpp>

Editor::Editor(ECS& ecs, Renderer& renderer)
    : _ecs(ecs)
    , _renderer(renderer)
{
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;

    const vk::Format format = renderer.GetSwapChain().GetFormat();
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = renderer.GetGBuffers().DepthFormat();

    ImGui_ImplVulkan_InitInfo initInfoVulkan {};
    initInfoVulkan.UseDynamicRendering = true;
    initInfoVulkan.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfo>(pipelineRenderingCreateInfoKhr);
    initInfoVulkan.PhysicalDevice = renderer.GetBrain().physicalDevice;
    initInfoVulkan.Device = renderer.GetBrain().device;
    initInfoVulkan.ImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfoVulkan.Instance = renderer.GetBrain().instance;
    initInfoVulkan.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfoVulkan.Queue = renderer.GetBrain().graphicsQueue;
    initInfoVulkan.QueueFamily = renderer.GetBrain().queueFamilyIndices.graphicsFamily.value();
    initInfoVulkan.DescriptorPool = renderer.GetBrain().descriptorPool;
    initInfoVulkan.MinImageCount = 2;
    initInfoVulkan.ImageCount = renderer.GetSwapChain().GetImageCount();
    ImGui_ImplVulkan_Init(&initInfoVulkan);

    ImGui_ImplVulkan_CreateFontsTexture();

    _basicSampler = util::CreateSampler(renderer.GetBrain(), vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 1);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    _entityEditor.registerComponent<TransformComponent>("Transform");
    _entityEditor.registerComponent<NameComponent>("Name");
    _entityEditor.registerComponent<RelationshipComponent>("Relationship");
    _entityEditor.registerComponent<WorldMatrixComponent>("WorldMatrix");
}

void Editor::Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    DrawMainMenuBar();
    // Hierarchy panel
    const auto displayEntity = [&](const auto& self, entt::entity entity) -> void
    {
        RelationshipComponent* relationship = _ecs._registry.try_get<RelationshipComponent>(entity);
        const std::string name = std::string(NameComponent::GetDisplayName(_ecs._registry, entity));
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
                    if (_ecs._registry.valid(current))
                    {
                        self(self, current);
                        current = _ecs._registry.get<RelationshipComponent>(current).next;
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
            entt::entity entity = _ecs._registry.create();

            _ecs._registry.emplace<TransformComponent>(entity);
        }

        if (ImGui::BeginChild("Hierarchy Panel"))
        {
            for (const auto [entity] : _ecs._registry.storage<entt::entity>().each())
            {
                RelationshipComponent* relationship = _ecs._registry.try_get<RelationshipComponent>(entity);

                if (relationship == nullptr || relationship->parent == entt::null)
                {
                    displayEntity(displayEntity, entity);
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();

    if (ImGui::Begin("Entity Details"))
    {
        DisplaySelectedEntityDetails();
    }
    ImGui::End();

    performanceTracker.Render();
    bloomSettings.Render();

    // Render systems inspect
    for (const auto& system : _ecs._systems)
    {
        system->Inspect();
    }
    DirectionalLight& light = scene.directionalLight;
    // for debug info
    static ImTextureID textureID = ImGui_ImplVulkan_AddTexture(_basicSampler.get(), _renderer.GetBrain().GetImageResourceManager().Access(_renderer.GetGBuffers().Shadow())->view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    ImGui::Begin("Light Debug");
    ImGui::DragFloat3("Position", &light.camera.position.x, 0.05f);
    ImGui::DragFloat3("Rotation", &light.camera.eulerRotation.x, 0.05f);
    ImGui::DragFloat("Ortho Size", &light.camera.orthographicSize, 0.1f);
    ImGui::DragFloat("Far Plane", &light.camera.farPlane, 0.1f);
    ImGui::DragFloat("Near Plane", &light.camera.nearPlane, 0.1f);
    ImGui::DragFloat("Shadow Bias", &light.shadowBias, 0.0001f);
    ImGui::Image(textureID, ImVec2(512, 512));
    ImGui::End();
    //

    ImGui::Begin("Dump VMA stats");

    if (ImGui::Button("Dump json"))
    {
        char* statsJson;
        vmaBuildStatsString(_renderer.GetBrain().vmaAllocator, &statsJson, true);

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

        vmaFreeStatsString(_renderer.GetBrain().vmaAllocator, statsJson);
    }

    ImGui::End();

    ImGui::Begin("Renderer Stats");

    ImGui::LabelText("Draw calls", "%i", _renderer.GetBrain().drawStats.drawCalls);
    ImGui::LabelText("Triangles", "%i", _renderer.GetBrain().drawStats.indexCount / 3);
    ImGui::LabelText("Indirect draw commands", "%i", _renderer.GetBrain().drawStats.indirectDrawCommands);
    ImGui::LabelText("Debug lines", "%i", _renderer.GetBrain().drawStats.debugLines);

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

    if (!_ecs._registry.valid(_selectedEntity))
    {
        ImGui::Text("Selected entity is not valid");
        return;
    }
    const std::string name = std::string(NameComponent::GetDisplayName(_ecs._registry, _selectedEntity));
    ImGui::LabelText("##EntityDetails", "%s", name.c_str());

    if (ImGui::Button("Delete"))
    {
        _ecs.DestroyEntity(_selectedEntity);
        _selectedEntity = entt::null;
        return;
    }
    ImGui::PushID(static_cast<int>(_selectedEntity));

    TransformComponent* transform = _ecs._registry.try_get<TransformComponent>(_selectedEntity);
    NameComponent* nameComponent = _ecs._registry.try_get<NameComponent>(_selectedEntity);
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
            TransformHelpers::UpdateWorldMatrix(_ecs._registry, _selectedEntity);
        }
    }

    if (nameComponent != nullptr)
    {
        ImGui::InputText("Name", &nameComponent->name);
    }

    ImGui::PopID();
    // inspect other components
}

Editor::~Editor() = default;
