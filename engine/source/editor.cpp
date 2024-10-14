#include "editor.hpp"

#include "imgui_impl_vulkan.h"
#include "application.hpp"
#include "performance_tracker.hpp"
#include "bloom_settings.hpp"
#include "mesh.hpp"

#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include "ECS.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "gbuffers.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#undef GLM_ENABLE_EXPERIMENTAL

Editor::Editor(const VulkanBrain& brain, Application& application, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers)
    : _brain(brain)
    , _application(application)
    , _gBuffers(gBuffers)
{
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &swapchainFormat;
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = depthFormat;

    ImGui_ImplVulkan_InitInfo initInfoVulkan {};
    initInfoVulkan.UseDynamicRendering = true;
    initInfoVulkan.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfo>(pipelineRenderingCreateInfoKhr);
    initInfoVulkan.PhysicalDevice = _brain.physicalDevice;
    initInfoVulkan.Device = _brain.device;
    initInfoVulkan.ImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfoVulkan.Instance = _brain.instance;
    initInfoVulkan.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfoVulkan.Queue = _brain.graphicsQueue;
    initInfoVulkan.QueueFamily = _brain.queueFamilyIndices.graphicsFamily.value();
    initInfoVulkan.DescriptorPool = _brain.descriptorPool;
    initInfoVulkan.MinImageCount = 2;
    initInfoVulkan.ImageCount = swapchainImages;
    ImGui_ImplVulkan_Init(&initInfoVulkan);

    ImGui_ImplVulkan_CreateFontsTexture();

    _basicSampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 1);
}

void Editor::Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene, ECS& ecs)
{
    ImGui_ImplVulkan_NewFrame();
    _application.NewImGuiFrame();
    ImGui::NewFrame();

    // Hierarchy panel
    const auto displayEntity = [&](const auto& self, entt::entity entity) -> void
    {
        RelationshipComponent* relationship = ecs._registry.try_get<RelationshipComponent>(entity);
        const std::string name = NameComponent::GetDisplayName(ecs._registry, entity);
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (relationship != nullptr && relationship->_children > 0)
        {
            const bool nodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<long long>(entity)), nodeFlags, "%s", name.c_str());

            if (ImGui::IsItemClicked())
            {
                _selectedEntity = entity;
            }
            if (nodeOpen)
            {
                entt::entity current = relationship->_first;
                for (size_t i {}; i < relationship->_children; ++i)
                {
                    if (ecs._registry.valid(current))
                    {
                        self(self, current);
                        current = ecs._registry.get<RelationshipComponent>(current)._next;
                    }
                }

                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<long long>(entity)), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", name.c_str());
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
            entt::entity entity = ecs._registry.create();

            ecs._registry.emplace<TransformComponent>(entity);
            ecs._registry.emplace<WorldMatrixComponent>(entity);
        }

        if (ImGui::BeginChild("Hierarchy Panel"))
        {
            for (const auto [entity] : ecs._registry.storage<entt::entity>().each())
            {
                RelationshipComponent* relationship = ecs._registry.try_get<RelationshipComponent>(entity);

                if (relationship == nullptr || relationship->_parent == entt::null)
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
        DisplaySelectedEntityDetails(ecs);
    }
    ImGui::End();

    performanceTracker.Render();
    bloomSettings.Render();

    DirectionalLight& light = scene.directionalLight;
    // for debug info
    static ImTextureID textureID = ImGui_ImplVulkan_AddTexture(_basicSampler.get(), _brain.GetImageResourceManager().Access(_gBuffers.Shadow())->view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    ImGui::Begin("Light Debug");
    ImGui::DragFloat3("Light dir", &light.lightDir.x, 0.05f);
    ImGui::DragFloat("scene distance", &light.sceneDistance, 0.05f);
    ImGui::DragFloat3("Target Position", &light.targetPos.x, 0.05f);
    ImGui::DragFloat("Ortho Size", &light.orthoSize, 0.1f);
    ImGui::DragFloat("Far Plane", &light.farPlane, 0.1f);
    ImGui::DragFloat("Near Plane", &light.nearPlane, 0.1f);
    ImGui::DragFloat("Shadow Bias", &light.shadowBias, 0.0001f);
    ImGui::Image(textureID, ImVec2(512, 512));
    ImGui::End();
    //

    ImGui::Begin("Scene");

    int32_t indexToRemove = -1;
    for (size_t i = 0; i < scene.gameObjects.size(); ++i)
    {
        glm::vec3 scale;
        glm::vec3 translation;
        glm::quat rotationQ;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(scene.gameObjects[i].transform, scale, rotationQ, translation, skew, perspective);
        glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQ));

        if (ImGui::BeginChild(i + 0xf00f, ImVec2 { 400, 110 }))
        {
            ImGui::Text("Gameobject %i", static_cast<uint32_t>(i));
            ImGui::DragFloat3("Position", &translation.x);
            ImGui::DragFloat3("Scale", &scale.x);
            ImGui::DragFloat3("Rotation", &rotation.x);

            glm::mat4 mTranslation = glm::translate(glm::mat4 { 1.0f }, translation);
            glm::mat4 mScale = glm::scale(glm::mat4 { 1.0f }, scale);
            glm::mat4 mRotation = glm::mat4 { glm::quat { glm::radians(rotation) } };

            scene.gameObjects[i].transform = mTranslation * mRotation * mScale;

            if (ImGui::Button("X"))
            {
                indexToRemove = i;
            }
        }
        ImGui::EndChildFrame();
    }

    if (indexToRemove != -1)
    {
        scene.gameObjects.erase(scene.gameObjects.begin() + indexToRemove);
    }

    uint32_t count = scene.gameObjects.size();
    if (ImGui::Button("Add model"))
    {
        glm::mat4 transform = glm::translate(glm::mat4 { 1.0f }, glm::vec3 { count * 7.0f, 0.0f, 0.0f });
        transform = glm::scale(transform, glm::vec3 { 10.0f });
        scene.gameObjects.emplace_back(transform, scene.models[1]);
    }

    ImGui::End();

    ImGui::Begin("Dump VMA stats");

    if (ImGui::Button("Dump json"))
    {
        char* statsJson;
        vmaBuildStatsString(_brain.vmaAllocator, &statsJson, true);

        const char* outputFilePath = "vma_stats.json";

        std::ofstream file { outputFilePath };
        if (file.is_open())
        {
            file << statsJson;

            file.close();
        }
        else
        {
            spdlog::error("Failed writing VMA stats to file!");
        }

        vmaFreeStatsString(_brain.vmaAllocator, statsJson);
    }

    ImGui::End();

    ImGui::Begin("Renderer Stats");

    ImGui::LabelText("Draw calls", "%i", _brain.drawStats.drawCalls);
    ImGui::LabelText("Triangles", "%i", _brain.drawStats.indexCount / 3);

    ImGui::End();

    {
        ZoneNamedN(zone, "ImGui Render", true);
        ImGui::Render();
    }
}
void Editor::DisplaySelectedEntityDetails(ECS& ecs)
{
    if (_selectedEntity == entt::null)
    {
        ImGui::Text("No entity selected");
        return;
    }

    if (!ecs._registry.valid(_selectedEntity))
    {
        ImGui::Text("Selected entity is not valid");
        return;
    }
    const std::string name = NameComponent::GetDisplayName(ecs._registry, _selectedEntity);
    ImGui::LabelText("##EntityDetails", "%s", name.c_str());

    if (ImGui::Button("Delete"))
    {
        ecs.DestroyEntity(_selectedEntity);
        _selectedEntity = entt::null;
        return;
    }
    ImGui::PushID(reinterpret_cast<void*>(_selectedEntity));

    TransformComponent* transform = ecs._registry.try_get<TransformComponent>(_selectedEntity);
    if (transform != nullptr)
    {
        int changed = 0;
        // Inspect Transform component
        changed += ImGui::DragFloat3("Position", &transform->_localPosition.x);
        changed += ImGui::DragFloat4("Rotation", &transform->_localRotation.w);
        changed += ImGui::DragFloat3("Scale", &transform->_localScale.x);

        if (changed > 0)
        {
            TransformHelpers::UpdateWorldMatrix(ecs._registry, _selectedEntity);
        }
    }

    ImGui::PopID();
    // inspect other components
}

Editor::~Editor()
{
    ImGui_ImplVulkan_Shutdown();
    _application.ShutdownImGui();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
