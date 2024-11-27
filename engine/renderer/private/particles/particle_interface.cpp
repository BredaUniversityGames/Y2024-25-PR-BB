#include "particles/particle_interface.hpp"
#include "particles/particle_util.hpp"

#include "components/name_component.hpp"
#include "ecs.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "model_loader.hpp"
#include "particles/emitter_component.hpp"
#include "resource_management/image_resource_manager.hpp"

#include "stb/stb_image.h"

ParticleInterface::ParticleInterface(const std::shared_ptr<GraphicsContext>& context, const std::shared_ptr<ECS>& ecs)
    : _context(context)
    , _ecs(ecs)
{
    LoadEmitterPresets();

    // fill ECS with emitters
    for (size_t i = 0; i < MAX_EMITTERS; i++)
    {
        auto entity = _ecs->registry.create();
        EmitterComponent emitterComponent;
        _ecs->registry.emplace<EmitterComponent>(entity, emitterComponent);
        auto& name = _ecs->registry.emplace<NameComponent>(entity);
        name.name = "Particle Emitter " + std::to_string(i);
    }
}

ParticleInterface::~ParticleInterface()
{
    for (auto const &i : _emitterImages)
    {
        _context->Resources()->ImageResourceManager().Destroy(i);
    }
}

void ParticleInterface::LoadEmitterPresets()
{
    // TODO: serialize emitter presets and load from file

    std::shared_ptr<GraphicsResources> resources = { _context->Resources() };

    // hardcoded test emitter preset for now
    Emitter emitter;
    emitter.position = glm::vec3(1.0f, 2.0f, 3.0f);
    emitter.count = 5;
    emitter.velocity = glm::vec3(1.0f, 5.0f, 1.0f);
    emitter.mass = 2.0f;
    emitter.rotationVelocity = glm::vec3(1.0f);
    emitter.maxLife = 5.0f;
    emitter.materialIndex = LoadEmitterImage("assets/textures/nogameplay.png");
    emitter.size = glm::vec2(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height) / static_cast<float>(glm::max(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height));
    _emitterPresets.emplace_back(emitter);
}

uint32_t ParticleInterface::LoadEmitterImage(const char* imagePath)
{
    int32_t width, height, numChannels;
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

void ParticleInterface::SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit)
{
    auto view = _ecs->registry.view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& emitterComponent = _ecs->registry.get<EmitterComponent>(entity);
        if (emitterComponent.timesToEmit == 0)
        {
            emitterComponent.timesToEmit = timesToEmit;
            emitterComponent.emitter = _emitterPresets[static_cast<int>(emitterPreset)];
            spdlog::info("Spawned emitter!");
            break;
        }
    }
}
