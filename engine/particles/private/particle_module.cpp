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

ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    _physics = &engine.GetModule<PhysicsModule>();
    _context = engine.GetModule<RendererModule>().GetRenderer()->GetContext();
    _ecs = &engine.GetModule<ECSModule>();

    return ModuleTickOrder::ePostRender;
}

void ParticleModule::Tick(MAYBE_UNUSED Engine& engine)
{
    const auto emitterView = _ecs->GetRegistry().view<EmitterComponent, RigidbodyComponent>();
    for (const auto entity : emitterView)
    {
        const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);
        if (_physics->bodyInterface->GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
        {
            _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
        }
    }

    const auto activeView = _ecs->GetRegistry().view<EmitterComponent, ActiveEmitterTag>();
    for (const auto entity : activeView)
    {
        auto& emitter = _ecs->GetRegistry().get<EmitterComponent>(entity);

        // first remove active tags from inactive emitters and continue
        if (emitter.emitOnce)
        {
            _ecs->GetRegistry().remove<EmitterComponent>(entity);
            _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
            continue;
        }

        // update position and velocity
        if (_ecs->GetRegistry().all_of<RigidbodyComponent>(entity))
        {
            const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);

            const auto joltTranslation = _physics->bodyInterface->GetWorldTransform(rb.bodyID).GetTranslation();
            emitter.emitter.position = glm::vec3(joltTranslation.GetX(), joltTranslation.GetY(), joltTranslation.GetZ());

            if (_physics->bodyInterface->GetMotionType(rb.bodyID) == JPH::EMotionType::Static)
            {
                _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
                continue;
            }
            if (_physics->bodyInterface->GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
            {
                JPH::Vec3 rbVelocity = _physics->bodyInterface->GetLinearVelocity(rb.bodyID);
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

void ParticleModule::LoadEmitterPresets()
{
    // TODO: serialize emitter presets and load from file
    auto resources = _context->Resources();

    CPUImage creation;
    creation.SetFlags(vk::ImageUsageFlagBits::eSampled);
    creation.FromPNG("assets/textures/yellow_orb_particle.png");
    creation.isHDR = false;
    auto image = _context->Resources()->ImageResourceManager().Create(creation);
    _emitterImages.emplace_back(image);

    // hardcoded test emitter preset for now
    EmitterPreset preset;
    preset.emitDelay = 0.2f;
    preset.mass = 2.0f;
    preset.rotationVelocity = glm::vec2(0.0f, 4.0f);
    preset.maxLife = 5.0f;
    preset.materialIndex = image.Index();
    preset.count = 10;
    preset.type = ParticleType::eBillboard;
    preset.flags = static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow);
    float biggestSize = glm::max(resources->ImageResourceManager().Access(image)->width, resources->ImageResourceManager().Access(image)->height);
    preset.size = glm::vec3(
        resources->ImageResourceManager().Access(image)->width / biggestSize,
        resources->ImageResourceManager().Access(image)->height / biggestSize, 0.0f);
    _emitterPresets.emplace_back(preset);
}

void ParticleModule::SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, SpawnEmitterFlagBits flags, glm::vec3 position, glm::vec3 velocity)
{
    auto& preset = _emitterPresets[static_cast<int>(emitterPreset)];

    Emitter emitter;
    emitter.count = preset.count;
    emitter.mass = preset.mass;
    emitter.size = preset.size;
    emitter.materialIndex = preset.materialIndex;
    emitter.maxLife = preset.maxLife;
    emitter.rotationVelocity = preset.rotationVelocity;
    emitter.flags = preset.flags;

    // Set position and velocity according to which components the entity already has
    if (_ecs->GetRegistry().all_of<RigidbodyComponent>(entity))
    {
        const auto& rb = _ecs->GetRegistry().get<RigidbodyComponent>(entity);

        const auto joltTranslation = _physics->bodyInterface->GetWorldTransform(rb.bodyID).GetTranslation();
        emitter.position = glm::vec3(joltTranslation.GetX(), joltTranslation.GetY(), joltTranslation.GetZ());

        if (_physics->bodyInterface->GetMotionType(rb.bodyID) != JPH::EMotionType::Static)
        {
            JPH::Vec3 rbVelocity = _physics->bodyInterface->GetLinearVelocity(rb.bodyID);
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

    EmitterComponent component;
    component.emitter = emitter;
    component.type = preset.type;
    component.maxEmitDelay = preset.emitDelay;
    component.currentEmitDelay = preset.emitDelay;
    component.emitOnce = emitOnce;

    _ecs->GetRegistry().emplace_or_replace<EmitterComponent>(entity, component);
    if (HasAnyFlags(flags, SpawnEmitterFlagBits::eIsActive) || emitOnce)
    {
        _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
    }
}
