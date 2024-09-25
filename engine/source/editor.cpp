#include "editor.hpp"

#include "imgui_impl_vulkan.h"
#include "application.hpp"
#include "performance_tracker.hpp"
#include "bloom_settings.hpp"
#include "mesh.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

Editor::Editor(const VulkanBrain& brain, Application& application, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages) : _brain(brain), _application(application)
{
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr{};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &swapchainFormat;
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = depthFormat;

    ImGui_ImplVulkan_InitInfo initInfoVulkan{};
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
}

void Editor::Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene)
{
    ImGui_ImplVulkan_NewFrame();
    _application.NewImGuiFrame();
    ImGui::NewFrame();

    performanceTracker.Render();
    bloomSettings.Render();

    ImGui::Begin("Scene");

    int32_t indexToRemove = -1;
    for(size_t i = 0; i < scene.gameObjects.size(); ++i)
    {
        glm::vec3 scale;
        glm::vec3 translation;
        glm::quat rotationQ;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(scene.gameObjects[i].transform, scale, rotationQ, translation, skew, perspective);
        glm::vec3 rotation = glm::degrees(glm::eulerAngles(rotationQ));

        if(ImGui::BeginChild(i + 0xf00f, ImVec2{400, 110}))
        {
            ImGui::Text("Gameobject %i", static_cast<uint32_t>(i));
            ImGui::DragFloat3("Position", &translation.x);
            ImGui::DragFloat3("Scale", &scale.x);
            ImGui::DragFloat3("Rotation", &rotation.x);

            glm::mat4 mTranslation = glm::translate(glm::mat4{1.0f}, translation);
            glm::mat4 mScale = glm::scale(glm::mat4{1.0f}, scale);
            glm::mat4 mRotation = glm::mat4{glm::quat{glm::radians(rotation)}};

            scene.gameObjects[i].transform = mTranslation * mRotation * mScale;

            if(ImGui::Button("X"))
            {
                indexToRemove = i;
            }
        }
        ImGui::EndChildFrame();
    }

    if(indexToRemove != -1)
    {
        scene.gameObjects.erase(scene.gameObjects.begin() + indexToRemove);
    }

    uint32_t count = scene.gameObjects.size();
    if(ImGui::Button("Add model"))
    {
        glm::mat4 transform = glm::translate(glm::mat4{ 1.0f }, glm::vec3{ count * 7.0f, 0.0f, 0.0f });
        transform = glm::scale(transform, glm::vec3{ 10.0f });
        scene.gameObjects.emplace_back(transform, scene.models[1]);
    }

    ImGui::End();

    {
        ZoneNamedN(zone, "ImGui Render", true);
        ImGui::Render();
    }
}

Editor::~Editor()
{
    ImGui_ImplVulkan_Shutdown();
    _application.ShutdownImGui();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
