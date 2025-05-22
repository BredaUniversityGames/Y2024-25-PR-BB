#include "wren_bindings.hpp"

#include "analytics/analytics_bindings.hpp"
#include "application_module.hpp"
#include "audio/audio_bindings.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "entity/entity_bind.hpp"
#include "game/game_bindings.hpp"
#include "game_module.hpp"
#include "gpu_scene.hpp"
#include "input/input_bindings.hpp"
#include "particle_module.hpp"
#include "particles/particle_bindings.hpp"
#include "passes/debug_pass.hpp"
#include "pathfinding/pathfinding_bindings.hpp"
#include "pathfinding_module.hpp"
#include "physics/physics_bindings.hpp"
#include "physics_module.hpp"
#include "renderer/animation_bindings.hpp"
#include "renderer_module.hpp"
#include "scene/model_loader.hpp"
#include "scripting_module.hpp"
#include "time_module.hpp"
#include "ui_module.hpp"
#include "utility/math_bind.hpp"
#include "utility/random_util.hpp"
#include "wren_engine.hpp"
#include "wren_entity.hpp"

namespace bindings
{

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
}

void TransitionToScript(WrenEngine& engine, const std::string& path)
{
    engine.instance->GetModule<GameModule>().SetNextScene(path);
}

WrenEntity LoadModelScripting(WrenEngine& engine, const std::string& path, bool loadWithCollision)
{
    auto& sceneCache = engine.instance->GetModule<GameModule>()._modelsLoaded;
    auto model = sceneCache.LoadModel(*engine.instance, path);

    auto entity = model->Instantiate(*engine.instance, loadWithCollision);
    return { entity, &engine.instance->GetModule<ECSModule>().GetRegistry() };
}

void PreloadModel(WrenEngine& engine, const std::string& path)
{
    auto& sceneCache = engine.instance->GetModule<GameModule>()._modelsLoaded;
    sceneCache.LoadModel(*engine.instance, path);
}

void SetExit(WrenEngine& engine, int code)
{
    engine.instance->SetExit(code);
}

void SpawnDecal(WrenEngine& engine, glm::vec3 normal, glm::vec3 position, glm::vec2 size, std::string albedoName)
{
    engine.instance->GetModule<RendererModule>().GetRenderer()->GetGPUScene().SpawnDecal(normal, position, size, albedoName);
}

void ResetDecals(WrenEngine& engine)
{
    engine.instance->GetModule<RendererModule>().GetRenderer()->GetGPUScene().ResetDecals();
}

void SetFog(WrenEngine& engine, float density)
{
    engine.instance->GetModule<RendererModule>().GetRenderer()->GetSettings().data.fog.density = density;
}

float GetFog(WrenEngine& engine)
{
    return engine.instance->GetModule<RendererModule>().GetRenderer()->GetSettings().data.fog.density;
}

void SetAmbientStrength(WrenEngine& engine, float strength)
{
    engine.instance->GetModule<RendererModule>().GetRenderer()->GetSettings().data.lighting.ambientStrength = strength;
}

float GetAmbientStrength(WrenEngine& engine)
{
    return engine.instance->GetModule<RendererModule>().GetRenderer()->GetSettings().data.lighting.ambientStrength;
}

bool IsDistribution(MAYBE_UNUSED WrenEngine& self)
{
#if DISTRIBUTION
    return true;
#else
    return false;
#endif
}

void DrawDebugLine(WrenEngine& engine, const glm::vec3& start, const glm::vec3& end)
{
    engine.instance->GetModule<RendererModule>().GetRenderer()->GetDebugPipeline().AddLine(start, end);
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
        engineAPI.func<&WrenEngine::GetModule<UIModule>>("GetUI");

        engineAPI.funcExt<bindings::LoadModelScripting>("LoadModel");
        engineAPI.funcExt<bindings::PreloadModel>("PreloadModel");
        engineAPI.funcExt<bindings::TransitionToScript>("TransitionToScript");
        engineAPI.funcExt<bindings::SetExit>("SetExit");
        engineAPI.funcExt<bindings::SpawnDecal>("SpawnDecal");
        engineAPI.funcExt<bindings::ResetDecals>("ResetDecals");
        engineAPI.propExt<bindings::GetFog, bindings::SetFog>("Fog");
        engineAPI.propExt<bindings::GetAmbientStrength, bindings::SetAmbientStrength>("AmbientStrength");
        engineAPI.funcExt<bindings::IsDistribution>("IsDistribution");
        engineAPI.funcExt<bindings::DrawDebugLine>("DrawDebugLine");
    }

    // Time Module
    {
        auto& time = module.klass<TimeModule>("TimeModule");
        time.funcExt<bindings::TimeModuleGetDeltatime>("GetDeltatime");
        time.func<&TimeModule::SetDeltatimeScale>("SetScale");
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

    // Analytics
    {
        BindAnalyticsAPI(module);
    }
}