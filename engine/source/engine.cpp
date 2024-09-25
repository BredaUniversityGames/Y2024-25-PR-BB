#include "engine.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vulkan_validation.hpp"
// #include "vulkan_helper.hpp"
#include "imgui_impl_vulkan.h"
#include "stopwatch.hpp"
#include "model_loader.hpp"
#include "util.hpp"
#include "mesh_primitives.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/lighting_pipeline.hpp"
#include "pipelines/skydome_pipeline.hpp"
#include "pipelines/tonemapping_pipeline.hpp"
#include "pipelines/ibl_pipeline.hpp"
#include "gbuffers.hpp"
#include "application.hpp"
#include "renderer.h"
#include "single_time_commands.hpp"

Engine::Engine(const InitInfo& initInfo, std::shared_ptr<Application> application)
{
    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());

    ImGui::CreateContext();
    ImPlot::CreateContext();
    spdlog::info("Starting engine...");

    _application = std::move(application);
    _renderer = std::make_unique<Renderer>(initInfo, _application);
    _scene = std::make_shared<SceneDescription>();
    _renderer->_scene = _scene;

    _scene->models.emplace_back(std::make_shared<ModelHandle>(_renderer->_modelLoader->Load("assets/models/DamagedHelmet.glb")));
    _scene->models.emplace_back(std::make_shared<ModelHandle>(_renderer->_modelLoader->Load("assets/models/ABeautifulGame/ABeautifulGame.gltf")));

    glm::vec3 scale { 0.05f };
    glm::mat4 rotation { glm::quat(glm::vec3(0.0f, 90.0f, 0.0f)) };
    glm::vec3 translate { -0.275f, 0.06f, -0.025f };
    glm::mat4 transform = glm::translate(glm::mat4 { 1.0f }, translate) * rotation * glm::scale(glm::mat4 { 1.0f }, scale);

    _scene->gameObjects.emplace_back(transform, _scene->models[0]);
    _scene->gameObjects.emplace_back(glm::mat4 { 1.0f }, _scene->models[1]);

    _scene->camera.position = glm::vec3 { 0.0f, 0.2f, 0.0f };
    _scene->camera.fov = glm::radians(45.0f);
    _scene->camera.nearPlane = 0.01f;
    _scene->camera.farPlane = 100.0f;

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    glm::ivec2 mousePos;
    _application->GetInputManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    _application->SetMouseHidden(true);

    spdlog::info("Successfully initialized engine!");
}

void Engine::Run()
{

    while (!ShouldQuit())
    {
        // update input
        ZoneNamed(zone, "");
        _application->ProcessEvents();
        auto currentFrameTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> deltaTime = currentFrameTime - _lastFrameTime;
        _lastFrameTime = currentFrameTime;
        float deltaTimeMS = deltaTime.count();

        // Slow down application when minimized.
        if (_application->IsMinimized())
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(16ms);
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        _application->NewImGuiFrame();
        ImGui::NewFrame();

        _performanceTracker.Render();

        {
            ZoneNamedN(zone, "Update Camera", true);
            int x, y;
            _application->GetInputManager().GetMousePosition(x, y);

            glm::ivec2 mouse_delta = glm::ivec2(x, y) - _lastMousePos;
            _lastMousePos = { x, y };

            constexpr float MOUSE_SENSITIVITY = 0.003f;
            constexpr float CAM_SPEED = 0.003f;

            constexpr glm::vec3 RIGHT = { 1.0f, 0.0f, 0.0f };
            constexpr glm::vec3 FORWARD = { 0.0f, 0.0f, 1.0f };
            // constexpr glm::vec3 UP = { 0.0f, -1.0f, 0.0f };

            _scene->camera.euler_rotation.x -= mouse_delta.y * MOUSE_SENSITIVITY;
            _scene->camera.euler_rotation.y -= mouse_delta.x * MOUSE_SENSITIVITY;

            glm::vec3 movement_dir {};
            if (_application->GetInputManager().IsKeyHeld(InputManager::Key::W))
                movement_dir -= FORWARD;

            if (_application->GetInputManager().IsKeyHeld(InputManager::Key::S))
                movement_dir += FORWARD;

            if (_application->GetInputManager().IsKeyHeld(InputManager::Key::D))
                movement_dir += RIGHT;

            if (_application->GetInputManager().IsKeyHeld(InputManager::Key::A))
                movement_dir -= RIGHT;

            if (glm::length(movement_dir) != 0.0f)
            {
                movement_dir = glm::normalize(movement_dir);
            }

            _scene->camera.position += glm::quat(_scene->camera.euler_rotation) * movement_dir * deltaTimeMS * CAM_SPEED;
        }

        if (_application->GetInputManager().IsKeyPressed(InputManager::Key::Escape))
            Quit();

        _renderer->UpdateCamera(_scene->camera);

        _renderer->Render();

        _performanceTracker.Update();

        FrameMark;
    }
}

Engine::~Engine()
{
    _renderer.reset();
}