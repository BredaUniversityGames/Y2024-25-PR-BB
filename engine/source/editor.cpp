#include "editor.hpp"

#include "imgui_impl_vulkan.h"
#include "application_module.hpp"
#include "performance_tracker.hpp"
#include "bloom_settings.hpp"
#include "mesh.hpp"
#include "modules/physics_module.hpp"
#include "profile_macros.hpp"
#include "log.hpp"

#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include "ECS.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "gbuffers.hpp"

#include <imgui_impl_sdl3.h>
#undef GLM_ENABLE_EXPERIMENTAL

Editor::Editor(const VulkanBrain& brain, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers)
    : _brain(brain)
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
    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    performanceTracker.Render();
    bloomSettings.Render();

    // Render systems inspect
    for (const auto& system : ecs._systems)
    {
        system->Inspect();
    }
    DirectionalLight& light = scene.directionalLight;
    // for debug info
    static ImTextureID textureID = ImGui_ImplVulkan_AddTexture(_basicSampler.get(), _brain.GetImageResourceManager().Access(_gBuffers.Shadow())->view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
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
            bblog::error("Failed writing VMA stats to file!");
        }

        vmaFreeStatsString(_brain.vmaAllocator, statsJson);
    }

    ImGui::End();

    ImGui::Begin("Renderer Stats");

    ImGui::LabelText("Draw calls", "%i", _brain.drawStats.drawCalls);
    ImGui::LabelText("Triangles", "%i", _brain.drawStats.indexCount / 3);
    ImGui::LabelText("Debug lines", "%i", _brain.drawStats.debugLines);

    ImGui::End();

    {
        ZoneNamedN(zone, "ImGui Render", true);
        ImGui::Render();
    }
}

Editor::~Editor()
{
}
