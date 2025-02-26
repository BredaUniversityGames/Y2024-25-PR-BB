#include "wren_bindings.hpp"

#include "application_module.hpp"
#include "audio/audio_bindings.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "entity/entity_bind.hpp"
#include "entity/wren_entity.hpp"
#include "game/game_bindings.hpp"
#include "game_module.hpp"
#include "input/input_bindings.hpp"
#include "particle_module.hpp"
#include "particles/particle_bindings.hpp"
#include "pathfinding/pathfinding_bindings.hpp"
#include "pathfinding_module.hpp"
#include "physics/physics_bindings.hpp"
#include "physics_module.hpp"
#include "renderer/animation_bindings.hpp"
#include "renderer/renderer_bindings.hpp"
#include "renderer_module.hpp"
#include "scene/scene_loader.hpp"
#include "scripting_module.hpp"
#include "time_module.hpp"
#include "utility/math_bind.hpp"
#include "utility/random_util.hpp"
#include "wren_engine.hpp"

#include <model_loading.hpp>

namespace bindings
{

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
}

void TransitionToScript(WrenEngine& engine, const std::string& path)
{
    engine.instance->GetModule<ScriptingModule>().SetMainScript(*engine.instance, path);
}

std::vector<WrenEntity> LoadModelIntoECS(WrenEngine& engine, const std::string& path)
{
    std::vector<entt::entity> entities = SceneLoading::LoadModels(*engine.instance, { path });
    std::vector<WrenEntity> wrentities(static_cast<size_t>(entities.size()));

    auto& registry = engine.GetModule<ECSModule>().value()->GetRegistry();

    for (size_t i = 0; i < entities.size(); i++)
    {
        wrentities[i].entity = entities[i];
        wrentities[i].registry = &registry;
    }

    return wrentities;
}

CPUModel LoadCPUModel(MAYBE_UNUSED WrenEngine& engine, const std::string& path)
{
    return ModelLoading::LoadGLTF(path);
}

}

void BindEngineAPI(wren::ForeignModule& module)
{
    bindings::BindMath(module);
    bindings::BindRandom(module);

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = module.klass<WrenEngine>("Engine");
        engineAPI.func<&WrenEngine::GetModule<TimeModule>>("GetTime");
        engineAPI.func<&WrenEngine::GetModule<ECSModule>>("GetECS");
        engineAPI.func<&WrenEngine::GetModule<ApplicationModule>>("GetInput");
        engineAPI.func<&WrenEngine::GetModule<AudioModule>>("GetAudio");
        engineAPI.func<&WrenEngine::GetModule<ParticleModule>>("GetParticles");
        engineAPI.func<&WrenEngine::GetModule<PhysicsModule>>("GetPhysics");
        engineAPI.func<&WrenEngine::GetModule<GameModule>>("GetGame");
        engineAPI.func<&WrenEngine::GetModule<PathfindingModule>>("GetPathfinding");
        engineAPI.func<&WrenEngine::GetModule<RendererModule>>("GetRenderer");
        engineAPI.funcExt<bindings::LoadModelIntoECS>("LoadModelIntoECS");
        engineAPI.funcExt<bindings::LoadCPUModel>("LoadCPUModel");
        engineAPI.funcExt<bindings::TransitionToScript>("TransitionToScript");
    }

    // Time Module
    {
        auto& time = module.klass<TimeModule>("TimeModule");
        time.funcExt<bindings::TimeModuleGetDeltatime>("GetDeltatime");
    }

    // ECS module
    {
        BindEntityAPI(module);
    }

    // Input
    {
        BindInputAPI(module);
    }

    // Audio
    {
        BindAudioAPI(module);
    }

    // Animations
    {
        BindAnimationAPI(module);
    }

    // Renderer
    {
        BindRendererAPI(module);
    }

    // Particles
    {
        BindParticleAPI(module);
    }

    // Physics
    {
        BindPhysicsAPI(module);
    }

    // Pathfinding
    {
        BindPathfindingAPI(module);
    }

    // Game
    {
        BindGameAPI(module);
    }
}