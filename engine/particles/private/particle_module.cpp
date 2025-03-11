#include "particle_module.hpp"

#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
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

    return ModuleTickOrder::ePreRender;
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

        // update position and velocity
        if (_ecs->GetRegistry().all_of<RigidbodyComponent>(entity))
        {
            const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);

            const auto joltTranslation = _physics->GetBodyInterface().GetWorldTransform(rb.bodyID).GetTranslation();
            emitter.emitter.position = glm::vec3(joltTranslation.GetX(), joltTranslation.GetY(), joltTranslation.GetZ());
            emitter.emitter.position += emitter.positionOffset;

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
            emitter.emitter.position += emitter.positionOffset;
        }

        // update timers
        const float deltaTime = static_cast<double>(engine.GetModule<TimeModule>().GetDeltatime().count() * 1e-3);
        emitter.currentEmitDelay -= deltaTime;
        for (auto& burst : emitter.bursts)
        {
            if (burst.startTime > 0.0f)
            {
                burst.startTime -= deltaTime;
            }
            else
            {
                burst.currentInterval -= deltaTime;
            }
        }
    }
}

ResourceHandle<GPUImage>& ParticleModule::GetEmitterImage(std::string fileName, bool& imageFound)
{
    auto got = _emitterImages.find(fileName);

    if (got == _emitterImages.end())
    {
        if (std::filesystem::exists("assets/textures/particles/" + fileName))
        {
            CPUImage creation;
            creation.SetFlags(vk::ImageUsageFlagBits::eSampled);
            creation.SetName(fileName);
            creation.FromPNG("assets/textures/particles/" + fileName);
            creation.isHDR = false;
            auto image = _context->Resources()->ImageResourceManager().Create(creation);
            auto& resource = _emitterImages.emplace(fileName, image).first->second;
            _context->UpdateBindlessSet();
            imageFound = true;
            return resource;
        }

        bblog::error("[Error] Image not found!");
        imageFound = false;
        return _emitterImages.begin()->second;
    }

    imageFound = true;
    return got->second;
}

bool ParticleModule::SetEmitterPresetImage(EmitterPreset& preset, std::string fileName)
{
    auto resources = _context->Resources();

    bool imageFound;
    auto image = GetEmitterImage(fileName, imageFound);

    preset.imageName = fileName;
    preset.materialIndex = image.Index();
    float biggestSize = glm::max(resources->ImageResourceManager().Access(image)->width, resources->ImageResourceManager().Access(image)->height);
    preset.size = glm::vec3(
        resources->ImageResourceManager().Access(image)->width / biggestSize,
        resources->ImageResourceManager().Access(image)->height / biggestSize, preset.size.z);

    return imageFound;
}

void ParticleModule::LoadEmitterPresets()
{
    // TODO: serialize emitter presets and load from file

    { // TEST
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
        SetEmitterPresetImage(preset, "jeremi.png");

        _emitterPresets.emplace_back(preset);
    }

    { // FLAME
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
        SetEmitterPresetImage(preset, "flame_03.png");
        preset.size.z = -0.8f;

        _emitterPresets.emplace_back(preset);
    }

    { // DUST
        EmitterPreset preset;
        preset.emitDelay = 0.5f;
        preset.mass = 0.005f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 8.0f;
        preset.count = 20;
        preset.spawnRandomness = glm::vec3(1.0f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        preset.name = "Dust";
        SetEmitterPresetImage(preset, "point_03.png");
        preset.size = glm::vec3(0.05f, 0.05f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    { // IMPACT
        EmitterPreset preset;
        preset.emitDelay = 0.1f;
        preset.mass = 0.5f;
        preset.rotationVelocity = glm::vec2(0.0f, 0.0f);
        preset.maxLife = 1.0f;
        preset.count = 100;
        preset.spawnRandomness = glm::vec3(5.0f, 0.0f, 5.0f);
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
        preset.color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f);
        preset.name = "Impact";
        SetEmitterPresetImage(preset, "Splatter-Sheet.png");
        preset.spriteDimensions = glm::ivec2(5, 6);
        preset.frameCount = 30;

        _emitterPresets.emplace_back(preset);
    }

    {
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
        SetEmitterPresetImage(preset, "swoosh.png");
        preset.size = glm::vec3(0.4f, 0.4f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    {
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
        SetEmitterPresetImage(preset, "star.png");
        preset.size = glm::vec3(0.2f, 0.2f, -0.03f);

        _emitterPresets.emplace_back(preset);
    }

    {
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
        SetEmitterPresetImage(preset, "swoosh.png");
        preset.size = glm::vec3(0.2f, 0.2f, 0.0f);

        _emitterPresets.emplace_back(preset);
    }

    { // FIRE SHEET
        EmitterPreset preset;
        preset.emitDelay = 2.0f;
        preset.mass = 0.0f;
        preset.maxLife = 2.0f;
        preset.count = 1;
        preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow | ParticleRenderFlagBits::eFrameBlend | ParticleRenderFlagBits::eLockY);
        preset.name = "SpriteSheetTest";
        preset.startingVelocity = glm::vec3(0.0f);
        SetEmitterPresetImage(preset, "Fire+Sparks-Sheet.png");
        preset.size *= 2.0f;
        preset.spriteDimensions = glm::ivec2(4, 5);
        preset.frameCount = 19;

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
    emitter.velocity = preset.startingVelocity;
    emitter.color = preset.color;
    emitter.maxFrames = preset.spriteDimensions;
    emitter.frameRate = preset.frameRate;
    emitter.frameCount = preset.frameCount;

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
    }
    else
    {
        emitter.position = glm::vec3(0.0f, 0.0f, 0.0f);
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
    component.count = emitter.count;
    std::copy(preset.bursts.begin(), preset.bursts.end(), std::back_inserter(component.bursts));

    _ecs->GetRegistry().emplace_or_replace<ParticleEmitterComponent>(entity, component);
    if (HasAnyFlags(flags, SpawnEmitterFlagBits::eIsActive) || emitOnce)
    {
        _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
    }
}

void ParticleModule::SpawnBurst(entt::entity entity, uint32_t count, float maxInterval, float startTime, bool loop, uint32_t cycles)
{
    ParticleBurst burst;
    burst.count = count;
    burst.maxInterval = maxInterval;
    burst.loop = loop;
    burst.cycles = cycles;
    burst.startTime = startTime;
    SpawnBurst(entity, std::move(burst));
}

void ParticleModule::SpawnBurst(entt::entity entity, const ParticleBurst& burst)
{
    if (_ecs->GetRegistry().all_of<ParticleEmitterComponent>(entity))
    {
        auto& emitter = _ecs->GetRegistry().get<ParticleEmitterComponent>(entity);
        emitter.bursts.emplace_back(burst);
    }
    else
    {
        bblog::error("Particle Emitter component from entity %i not found!", static_cast<size_t>(entity));
    }
}
