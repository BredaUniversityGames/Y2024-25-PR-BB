#include "particles/particle_util.hpp"
#include "particles/particle_interface.hpp"

#include "ECS.hpp"
#include "model_loader.hpp"
#include "particles/emitter_component.hpp"

#include "stb/stb_image.h"

ParticleInterface::ParticleInterface(const VulkanBrain& brain, ECS& ecs)
    : _brain(brain)
    , _ecs(ecs)
{
    // TODO: later, serialize emitter presets and load from file here
    // hardcoded test emitter preset for now
    Emitter emitter;
    emitter.position = glm::vec3(1.0f, 2.0f, 3.0f);
    emitter.count = 5;
    emitter.velocity = glm::vec3(1.0f, 5.0f, 1.0f);
    emitter.mass = 2.0f;
    emitter.rotationVelocity = glm::vec3(1.0f);
    emitter.maxLife = 5.0f;
    emitter.materialIndex = LoadEmitterImage("assets/textures/nogameplay.png");
    _emitterPresets.emplace_back(emitter);

    // fill ECS with emitters
    for (size_t i = 0; i < MAX_EMITTERS; i++)
    {
        auto entity = _ecs._registry.create();
        EmitterComponent emitterComponent;
        _ecs._registry.emplace<EmitterComponent>(entity, emitterComponent);
    }
}

ParticleInterface::~ParticleInterface()
{
    for(auto i : _emitterImages)
    {
        _brain.GetImageResourceManager().Destroy(i);
    }
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

    ImageCreation creation {};
    creation.SetSize(width, height).SetFlags(vk::ImageUsageFlagBits::eSampled).SetName("Emitter Image").SetData(data.data()).SetFormat(vk::Format::eR8G8B8A8Unorm);

    auto image = _brain.GetImageResourceManager().Create(creation);
    _emitterImages.emplace_back(image);

    return image.index;
}

void ParticleInterface::SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit)
{
    auto view = _ecs._registry.view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& emitterComponent = _ecs._registry.get<EmitterComponent>(entity);
        if (emitterComponent.timesToEmit == 0)
        {
            emitterComponent.timesToEmit = timesToEmit;
            emitterComponent.emitter = _emitterPresets[static_cast<int>(emitterPreset)];
            spdlog::info("Spawned emitter!");
            break;
        }
    }
}
