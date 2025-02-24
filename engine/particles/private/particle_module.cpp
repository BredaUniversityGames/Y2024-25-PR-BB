#include "particle_module.hpp"

#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "emitter_component.hpp"
#include "engine.hpp"
#include "gpu_resources.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "physics_module.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "time_module.hpp"

#include <filesystem>

ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    _physics = &engine.GetModule<PhysicsModule>();
    _context = engine.GetModule<RendererModule>().GetRenderer()->GetContext();
    _ecs = &engine.GetModule<ECSModule>();

    return ModuleTickOrder::ePostRender;
}

void ParticleModule::Tick(MAYBE_UNUSED Engine& engine)
{
    const auto emitterView = _ecs->GetRegistry().view<ParticleEmitterComponent, RigidbodyComponent>();
    for (const auto entity : emitterView)
    {
        const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);
        if (_physics->GetBodyInterface().GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
        {
            _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
        }
    }

    const auto activeView = _ecs->GetRegistry().view<ParticleEmitterComponent, ActiveEmitterTag>();
    for (const auto entity : activeView)
    {
        auto& emitter = _ecs->GetRegistry().get<ParticleEmitterComponent>(entity);

        // first remove active tags from inactive emitters and continue
        if (emitter.emitOnce)
        {
            _ecs->GetRegistry().remove<ParticleEmitterComponent>(entity);
            _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
            continue;
        }

        // update position and velocity
        if (_ecs->GetRegistry().all_of<RigidbodyComponent>(entity))
        {
            const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);

            const auto joltTranslation = _physics->GetBodyInterface().GetWorldTransform(rb.bodyID).GetTranslation();
            emitter.emitter.position = glm::vec3(joltTranslation.GetX(), joltTranslation.GetY(), joltTranslation.GetZ());

            if (_physics->GetBodyInterface().GetMotionType(rb.bodyID) == JPH::EMotionType::Static)
            {
                _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
                continue;
            }
            if (_physics->GetBodyInterface().GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
            {
                JPH::Vec3 rbVelocity = _physics->GetBodyInterface().GetLinearVelocity(rb.bodyID);
                emitter.emitter.velocity = -glm::vec3(rbVelocity.GetX(), rbVelocity.GetY(), rbVelocity.GetZ());
            }
        }
        else if (_ecs->GetRegistry().all_of<WorldMatrixComponent>(entity))
        {
            emitter.emitter.position = TransformHelpers::GetWorldPosition(_ecs->GetRegistry(), entity);
        }

        // update timers
        if (emitter.currentEmitDelay < 0.0f)
        {
            emitter.currentEmitDelay = emitter.maxEmitDelay;
        }
        emitter.currentEmitDelay -= engine.GetModule<TimeModule>().GetDeltatime().count() * 1e-3;
    }
}

ResourceHandle<GPUImage>& ParticleModule::GetEmitterImage(std::string fileName)
{
    auto got = _emitterImages.find(fileName);

    if (got == _emitterImages.end())
    {
        if (std::filesystem::exists("assets/textures/" + fileName))
        {
            CPUImage creation;
            creation.SetFlags(vk::ImageUsageFlagBits::eSampled);
            creation.SetName(fileName);
            creation.FromPNG("assets/textures/" + fileName);
            creation.isHDR = false;
            auto image = _context->Resources()->ImageResourceManager().Create(creation);
            auto& resource = _emitterImages.emplace(fileName, image).first->second;
            _context->UpdateBindlessSet();
            return resource;
        }

        bblog::error("[Error] Image not found!");
        return _emitterImages.begin()->second;
    }

    return got->second;
}

void ParticleModule::SetEmitterPresetImage(EmitterPreset& preset, ResourceHandle<GPUImage> image)
{
    auto resources = _context->Resources();

    preset.materialIndex = image.Index();
    float biggestSize = glm::max(resources->ImageResourceManager().Access(image)->width, resources->ImageResourceManager().Access(image)->height);
    preset.size = glm::vec3(
        resources->ImageResourceManager().Access(image)->width / biggestSize,
        resources->ImageResourceManager().Access(image)->height / biggestSize, preset.size.z);
}

void ParticleModule::LoadEmitterPresets()
{
    // TODO: serialize emitter presets and load from file

    { // TEST
        auto image = GetEmitterImage("jeremi.png");

        EmitterPreset preset;
        preset.emitDelay = 0.2f;
        preset.mass = 2.0f;
        preset.rotationVelocity = glm::vec2(0.0f, 4.0f);
        preset.maxLife = 5.0f;
        preset.count = 10;
        preset.spawnRandomness = glm::vec3(1.0f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(1.0f);
        preset.name = "Test";
        SetEmitterPresetImage(preset, image);

        _emitterPresets.emplace_back(preset);
    }

    { // FLAME
        auto image = GetEmitterImage("flame_03.png");

        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = -0.005f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 2.0f;
        preset.count = 10;
        preset.spawnRandomness = glm::vec3(0.05f, 0.05f, 0.05f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(1.0f, 1.0f, 1.0f, 5.0f);
        preset.name = "Flame";
        preset.velocityRandomness = glm::vec3(0.05f, 0.05f, 0.05f);
        SetEmitterPresetImage(preset, image);
        preset.size.z = -0.8f;

        _emitterPresets.emplace_back(preset);
    }

    { // DUST
        auto image = GetEmitterImage("point_03.png");

        EmitterPreset preset;
        preset.emitDelay = 1.0f;
        preset.mass = 0.005f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 8.0f;
        preset.count = 20;
        preset.spawnRandomness = glm::vec3(1.0f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        preset.name = "Dust";
        SetEmitterPresetImage(preset, image);
        preset.size = glm::vec3(0.05f, 0.05f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    {
        // TODO: serialize emitter presets and load from file
        auto image = GetEmitterImage("star.png");

        // hardcoded test emitter preset for now
        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = 1.0f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 1.0f;
        preset.count = 100;
        preset.spawnRandomness = glm::vec3(5.0f, 0.0f, 5.0f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(0.2f, 0.2f, 1.0f, 1.0f);
        preset.name = "Impact";
        SetEmitterPresetImage(preset, image);
        preset.size = glm::vec3(2.0f, 2.0f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    {
        // TODO: serialize emitter presets and load from file
        auto image = GetEmitterImage("swoosh.png");

        // hardcoded test emitter preset for now
        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = 0.0f;
        preset.rotationVelocity = glm::vec2(0.0f, 10.0f);
        preset.maxLife = 1.0f;
        preset.count = 7;
        preset.spawnRandomness = glm::vec3(0.2f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(0.1f, 0.15f, 1.0f, 1.0f);
        preset.name = "Ray";
        SetEmitterPresetImage(preset, image);
        preset.size = glm::vec3(0.4f, 0.4f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    {
        auto image = GetEmitterImage("star.png");

        // hardcoded test emitter preset for now
        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = 0.0f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 0.3f;
        preset.count = 20;
        preset.spawnRandomness = glm::vec3(0.5f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(4.0f, 0.0f, 0.0f, 1.0f);
        preset.name = "Stab";
        SetEmitterPresetImage(preset, image);
        preset.size = glm::vec3(0.2f, 0.2f, -0.03f);

        _emitterPresets.emplace_back(preset);
    }

    {
        // TODO: serialize emitter presets and load from file
        auto image = GetEmitterImage("swoosh.png");

        // hardcoded test emitter preset for now
        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = 0.0f;
        preset.rotationVelocity = glm::vec2(0.0f, 10.0f);
        preset.maxLife = 1.0f;
        preset.count = 5;
        preset.spawnRandomness = glm::vec3(0.1f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
        preset.name = "ShotgunShoot";
        SetEmitterPresetImage(preset, image);
        preset.size = glm::vec3(0.2f, 0.2f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }
}

void ParticleModule::SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, SpawnEmitterFlagBits flags, glm::vec3 position, glm::vec3 velocity)
{
    SpawnEmitter(entity, static_cast<int32_t>(emitterPreset), flags, position, velocity);
}

void ParticleModule::SpawnEmitter(entt::entity entity, int32_t emitterPresetID, SpawnEmitterFlagBits flags, glm::vec3 position, glm::vec3 velocity)
{
    if (emitterPresetID > static_cast<int32_t>(_emitterPresets.size()) - 1)
        return;

    auto& preset = _emitterPresets[emitterPresetID];

    Emitter emitter;
    emitter.count = preset.count;
    emitter.mass = preset.mass;
    emitter.size = preset.size;
    emitter.materialIndex = preset.materialIndex;
    emitter.maxLife = preset.maxLife;
    emitter.rotationVelocity = preset.rotationVelocity;
    emitter.flags = preset.flags;
    emitter.spawnRandomness = preset.spawnRandomness;
    emitter.velocityRandomness = preset.velocityRandomness;
    emitter.color = preset.color;

    // Set position and velocity according to which components the entity already has
    if (_ecs->GetRegistry().all_of<RigidbodyComponent>(entity))
    {
        const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);

        const auto joltTranslation = _physics->GetBodyInterface().GetWorldTransform(rb.bodyID).GetTranslation();
        emitter.position = glm::vec3(joltTranslation.GetX(), joltTranslation.GetY(), joltTranslation.GetZ());

        if (_physics->GetBodyInterface().GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
        {
            JPH::Vec3 rbVelocity = _physics->GetBodyInterface().GetLinearVelocity(rb.bodyID);
            emitter.velocity = -glm::vec3(rbVelocity.GetX(), rbVelocity.GetY(), rbVelocity.GetZ());
            _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
        }
    }
    else if (_ecs->GetRegistry().all_of<WorldMatrixComponent>(entity))
    {
        emitter.position = TransformHelpers::GetWorldPosition(_ecs->GetRegistry(), entity);
        emitter.velocity = glm::vec3(1.0f, 5.0f, 1.0f);
    }
    else
    {
        emitter.position = glm::vec3(0.0f, 0.0f, 0.0f);
        emitter.velocity = glm::vec3(1.0f, 5.0f, 1.0f);
    }

    if (HasAnyFlags(flags, SpawnEmitterFlagBits::eSetCustomPosition))
    {
        emitter.position = position;
    }
    if (HasAnyFlags(flags, SpawnEmitterFlagBits::eSetCustomVelocity))
    {
        emitter.velocity = velocity;
    }
    bool emitOnce = HasAnyFlags(flags, SpawnEmitterFlagBits::eEmitOnce);

    ParticleEmitterComponent component;
    component.emitter = emitter;
    component.maxEmitDelay = preset.emitDelay;
    component.currentEmitDelay = preset.emitDelay;
    component.emitOnce = emitOnce;

    _ecs->GetRegistry().emplace_or_replace<ParticleEmitterComponent>(entity, component);
    if (HasAnyFlags(flags, SpawnEmitterFlagBits::eIsActive) || emitOnce)
    {
        _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
    }
}
