#include "particle_interface.hpp"
#include "particle_util.hpp"

#include "components/name_component.hpp"
#include "ecs_module.hpp"
#include "emitter_component.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "model_loader.hpp"
#include "resource_management/image_resource_manager.hpp"

#include "stb_image.h"

ParticleInterface::ParticleInterface(const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs)
    : _context(context)
    , _ecs(ecs)
{
    // fill ECS with emitters
    for (size_t i = 0; i < MAX_EMITTERS; i++)
    {
        auto entity = _ecs.GetRegistry().create();
        EmitterComponent emitterComponent;
        _ecs.GetRegistry().emplace<EmitterComponent>(entity, emitterComponent);
        auto& name = _ecs.GetRegistry().emplace<NameComponent>(entity);
        name.name = "Particle Emitter " + std::to_string(i);
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
    emitter.rotationVelocity = glm::vec2(0.0f, 4.0f);
    emitter.maxLife = 5.0f;
    emitter.materialIndex = LoadEmitterImage("assets/textures/jeremi.png");
    emitter.size = glm::vec2(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height) / static_cast<float>(glm::max(resources->ImageResourceManager().Access(_emitterImages[0])->width, resources->ImageResourceManager().Access(_emitterImages[0])->height));
    _emitterPresets.emplace_back(emitter);
}

uint32_t ParticleInterface::LoadEmitterImage(const char* imagePath)
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

void ParticleInterface::SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit)
{
    auto view = _ecs.GetRegistry().view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& emitterComponent = _ecs.GetRegistry().get<EmitterComponent>(entity);
        if (emitterComponent.timesToEmit == 0)
        {
            emitterComponent.timesToEmit = timesToEmit;
            emitterComponent.emitter = _emitterPresets[static_cast<int>(emitterPreset)];
            break;
        }
    }
}
