#include "particle_module.hpp"

#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "emitter_component.hpp"
#include "engine.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "physics_module.hpp"
#include "renderer.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "time_module.hpp"

#include <renderer_module.hpp>
#include <stb_image.h>


ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    _context = engine.GetModule<RendererModule>().GetRenderer()->GetContext();
    _ecs = &engine.GetModule<ECSModule>();

    LoadEmitterPresets();

    return ModuleTickOrder::ePostRender;
}

void ParticleModule::Tick(MAYBE_UNUSED Engine& engine)
{
    const auto emitterView = _ecs->GetRegistry().view<EmitterComponent>();
    for(const auto entity : emitterView)
    {
        auto rb = _ecs->GetRegistry().try_get<RigidbodyComponent>(entity);

        if(rb)
        {
            if(_physics->bodyInterface->GetMotionType(rb->bodyID) == JPH::EMotionType::Dynamic)
            {
                _ecs->GetRegistry().emplace_or_replace<ActiveEmitterTag>(entity);
            }
        }
    }

    const auto activeView = _ecs->GetRegistry().view<EmitterComponent, ActiveEmitterTag>();
    for(const auto entity : activeView)
    {
        auto& emitter = _ecs->GetRegistry().get<EmitterComponent>(entity);
        const auto rb = _ecs->GetRegistry().try_get<RigidbodyComponent>(entity);

        // first remove active tags from inactive emitters and continue
        if(emitter.emitOnce)
        {
            _ecs->GetRegistry().remove<EmitterComponent>(entity);
            _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
            continue;
        }
        if(rb)
        {
            if(_physics->bodyInterface->GetMotionType(rb->bodyID) != JPH::EMotionType::Dynamic)
            {
                _ecs->GetRegistry().remove<ActiveEmitterTag>(entity);
                continue;
            }
        }

        // update timers
        if(emitter.currentEmitDelay < 0.0f)
        {
            emitter.currentEmitDelay += emitter.maxEmitDelay;
        }
        emitter.currentEmitDelay -= engine.GetModule<TimeModule>().GetDeltatime().count() * 1e-3;

        // update position and velocity
        emitter.emitter.position = TransformHelpers::GetLocalPosition(_ecs->GetRegistry().get<TransformComponent>(entity));
        if(rb)
        {
            JPH::Vec3 rbVelocity = _physics->bodyInterface->GetLinearVelocity(rb->bodyID);
            emitter.emitter.velocity = -glm::vec3(rbVelocity.GetX(), rbVelocity.GetY(), rbVelocity.GetZ());
        }
    }
}

void ParticleModule::LoadEmitterPresets()
{
    // TODO: serialize emitter presets and load from file
    auto resources = _context->Resources();

    // hardcoded test emitter preset for now
    EmitterPreset preset;
    preset.emitDelay = 2.0f;
    preset.mass = 2.0f;
    preset.rotationVelocity = glm::vec2(0.0f, 4.0f);
    preset.maxLife = 5.0f;
    preset.materialIndex = LoadEmitterImage("assets/textures/jeremi.png");
    preset.count = 5;
    preset.type = ParticleType::eBillboard;
    preset.size = glm::vec3(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height / static_cast<float>(glm::max(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height)), 0.0f);
    _emitterPresets.emplace_back(preset);
}

uint32_t ParticleModule::LoadEmitterImage(const char* imagePath)
{
    int32_t width = 0, height = 0, numChannels = 0;
    void* stbiData = stbi_load(imagePath, &width, &height, &numChannels, 4);

    if (stbiData == nullptr)
        throw std::runtime_error("Failed loading Emitter Image!");

    std::vector<std::byte> data(width * height * 4);
    std::memcpy(data.data(), stbiData, data.size());

    stbi_image_free(stbiData);

    CPUImage creation {};
    creation.SetSize(width, height).SetFlags(vk::ImageUsageFlagBits::eSampled).SetName("Emitter Image").SetData(std::move(data)).SetFormat(vk::Format::eR8G8B8A8Unorm);

    auto image = _context->Resources()->ImageResourceManager().Create(creation);
    _emitterImages.emplace_back(image);

    return image.Index();
}

void ParticleModule::SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, bool emitOnce, bool isActive)
{
    auto preset = _emitterPresets[static_cast<int>(emitterPreset)];
    auto rb = _ecs->GetRegistry().try_get<RigidbodyComponent>(entity);

    Emitter emitter;
    emitter.count = preset.count;
    emitter.mass = preset.mass;
    emitter.size = preset.size;
    emitter.materialIndex = preset.materialIndex;
    emitter.maxLife = preset.maxLife;
    emitter.rotationVelocity = preset.rotationVelocity;
    // TODO: get world instead of local?
    emitter.position = TransformHelpers::GetLocalPosition(_ecs->GetRegistry().get<TransformComponent>(entity));
    if(rb)
    {
        JPH::Vec3 rbVelocity = _physics->bodyInterface->GetLinearVelocity(rb->bodyID);
        emitter.velocity = -glm::vec3(rbVelocity.GetX(), rbVelocity.GetY(), rbVelocity.GetZ());
    }
    else
    {
        emitter.velocity = glm::vec3(1.0f, 5.0f, 1.0f);
    }

    EmitterComponent component;
    component.emitter = emitter;
    component.type = preset.type;
    component.maxEmitDelay = preset.emitDelay;
    component.currentEmitDelay = preset.emitDelay;
    component.emitOnce = emitOnce;

    _ecs->GetRegistry().emplace<EmitterComponent>(entity, component);

    if(rb)
    {
        if(_physics->bodyInterface->GetMotionType(rb->bodyID) == JPH::EMotionType::Dynamic)
        {
            _ecs->GetRegistry().emplace<ActiveEmitterTag>(entity);
        }
    }
    else if(isActive)
    {
        _ecs->GetRegistry().emplace<ActiveEmitterTag>(entity);
    }
}
