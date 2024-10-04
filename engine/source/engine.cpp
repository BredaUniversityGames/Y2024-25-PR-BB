#include "engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include "ECS.hpp"

#include <stb_image.h>
#include <tracy/Tracy.hpp>
#include "vulkan_helper.hpp"
#include "imgui_impl_vulkan.h"
#include "model_loader.hpp"
#include "gbuffers.hpp"
#include "application.hpp"
#include "renderer.hpp"
#include "editor.hpp"
#include <implot/implot.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Engine::Engine(const InitInfo& initInfo, std::shared_ptr<Application> application)
{
    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    spdlog::info("Starting engine...");

    _application = std::move(application);
    _renderer = std::make_unique<Renderer>(initInfo, _application);

    _ecs = std::make_unique<ECS>();

    _scene = std::make_shared<SceneDescription>();
    _renderer->_scene = _scene;

    std::vector<std::string> modelPaths = {
        //"assets/models/DamagedHelmet.glb",
        "assets/models/ABeautifulGame/ABeautifulGame.gltf"
    };

    _scene->models = _renderer->FrontLoadModels(modelPaths);

    glm::vec3 scale { 10.0f };
    for (size_t i = 0; i < 10; ++i)
    {
        glm::vec3 translate { i / 3, 0.0f, i % 3 };
        glm::mat4 transform = glm::translate(glm::mat4 { 1.0f }, translate * 7.0f) * glm::scale(glm::mat4 { 1.0f }, scale);

        _scene->gameObjects.emplace_back(transform, _scene->models[0]);
    }

    _renderer->UpdateBindless();

    _editor = std::make_unique<Editor>(_renderer->_brain, *_application, _renderer->_swapChain->GetFormat(), _renderer->_gBuffers->DepthFormat(), _renderer->_swapChain->GetImageCount(), *_renderer->_gBuffers);

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
        _application->ProcessWindowEvents();
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

        int32_t mouseX, mouseY;
        _application->GetInputManager().GetMousePosition(mouseX, mouseY);

        if (_application->GetInputManager().IsKeyPressed(InputManager::Key::H))
            _application->SetMouseHidden(!_application->GetMouseHidden());

        if (_application->GetMouseHidden())
        {
            ZoneNamedN(zone, "Update Camera", true);

            glm::ivec2 mouse_delta = glm::ivec2 { mouseX, mouseY } - _lastMousePos;

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
        _lastMousePos = { mouseX, mouseY };

        if (_application->GetInputManager().IsKeyPressed(InputManager::Key::Escape))
            Quit();

        _ecs->UpdateSystems(deltaTimeMS);
        _ecs->RemovedDestroyed();
        _ecs->RenderSystems();

        _renderer->UpdateCamera(_scene->camera);

        _editor->Draw(_performanceTracker, _renderer->_bloomSettings, *_scene);

        _renderer->Render();

        _performanceTracker.Update();

        FrameMark;
    }
}

Engine::~Engine()
{
    _renderer->_brain.device.waitIdle();

    _editor.reset();
    _renderer.reset();
    _ecs.reset();
}
