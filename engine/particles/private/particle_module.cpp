#include "particle_module.hpp"

#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "engine.hpp"
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

    const auto testView = _ecs->GetRegistry().view<ParticleEmitterComponent, ActiveEmitterTag, TestEmitterTag>();
    for (const auto entity : testView)
    {
        auto& testEmitter = _ecs->GetRegistry().get<ParticleEmitterComponent>(entity);
        auto& preset = _emitterPresets.data.emitterPresets[static_cast<uint8_t>(testEmitter.presetID)];

        testEmitter.emitter.count = preset.count;
        testEmitter.emitter.mass = preset.mass;
        testEmitter.emitter.size = preset.size;
        testEmitter.emitter.materialIndex = preset.materialIndex;
        testEmitter.emitter.maxLife = preset.maxLife;
        testEmitter.emitter.rotationVelocity = preset.rotationVelocity;
        testEmitter.emitter.flags = preset.flags;
        testEmitter.emitter.spawnRandomness = preset.spawnRandomness;
        testEmitter.emitter.velocityRandomness = preset.velocityRandomness;
        testEmitter.emitter.velocity = preset.startingVelocity;
        testEmitter.emitter.color = preset.color;
        testEmitter.emitter.maxFrames = preset.spriteDimensions;
        testEmitter.emitter.frameRate = preset.frameRate;
        testEmitter.emitter.frameCount = preset.frameCount;
        testEmitter.maxEmitDelay = preset.emitDelay;
        testEmitter.count = preset.count;

        testEmitter.bursts.clear();
        std::copy(preset.bursts.begin(), preset.bursts.end(), std::back_inserter(testEmitter.bursts));
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

        bblog::error("[Error] Particle image %s not found!", fileName);
        imageFound = false;
        return _emitterImages.begin()->second;
    }
    imageFound = true;
    return got->second;
}

bool ParticleModule::SetEmitterPresetImage(EmitterPreset& preset)
{
    auto resources = _context->Resources();

    bool imageFound;
    auto image = GetEmitterImage(preset.imageName, imageFound);

    preset.materialIndex = image.Index();
    float biggestSize = glm::max(resources->ImageResourceManager().Access(image)->width, resources->ImageResourceManager().Access(image)->height);
    preset.size = glm::vec3(
        resources->ImageResourceManager().Access(image)->width / preset.spriteDimensions.x / biggestSize,
        resources->ImageResourceManager().Access(image)->height / preset.spriteDimensions.y / biggestSize, preset.size.z);

    return imageFound;
}

void ParticleModule::LoadEmitterPresets()
{
    for (size_t i = 0; i < _emitterPresets.data.emitterPresets.size(); ++i)
    {
        bool imageFound;
        _emitterPresets.data.emitterPresets[i].materialIndex = GetEmitterImage(_emitterPresets.data.emitterPresets[i].imageName, imageFound).Index();
        if (!imageFound)
        {
            bblog::error("[Particles] Emitter Preset image %s not found", _emitterPresets.data.emitterPresets[i].imageName);
        }
    }
}

void ParticleModule::SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, SpawnEmitterFlagBits flags, glm::vec3 position, glm::vec3 velocity)
{
    SpawnEmitter(entity, static_cast<int32_t>(emitterPreset), flags, position, velocity);
}

void ParticleModule::SpawnEmitter(entt::entity entity, int32_t emitterPresetID, SpawnEmitterFlagBits flags, glm::vec3 position, glm::vec3 velocity)
{
    if (emitterPresetID > static_cast<int32_t>(_emitterPresets.data.emitterPresets.size()) - 1)
        return;

    auto& preset = _emitterPresets.data.emitterPresets[emitterPresetID];

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
    emitter.id = emitterCount;

    emitterCount++;

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
    component.currentEmitDelay = 0.0f;
    component.emitOnce = emitOnce;
    component.count = emitter.count;
    component.presetID = static_cast<EmitterPresetID>(emitterPresetID);
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
