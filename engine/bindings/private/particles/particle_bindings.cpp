#include "particle_bindings.hpp"

#include "particle_module.hpp"
#include "utility/enum_bind.hpp"
#include "wren_entity.hpp"


namespace bindings
{
void LoadEmitterPresets(ParticleModule& self)
{
    self.LoadEmitterPresets();
}
void SpawnEmitter(ParticleModule& self, WrenEntity& entity, EmitterPresetID emitterPreset, uint8_t flags, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, glm::vec3 velocity = { 0.0f, 0.0f, 0.0f })
{
    self.SpawnEmitter(entity.entity, emitterPreset, static_cast<SpawnEmitterFlagBits>(flags), position, velocity);
}
void SpawnBurst(ParticleModule& self, WrenEntity& entity, uint32_t count, float maxInterval, float startTime = 0.0f, bool loop = true, uint32_t cycles = 0)
{
    self.SpawnBurst(entity.entity, count, maxInterval, startTime, loop, cycles);
}
}

void BindParticleAPI(wren::ForeignModule& module)
{
    bindings::BindEnum<EmitterPresetID>(module, "EmitterPresetID");
    bindings::BindBitflagEnum<SpawnEmitterFlagBits>(module, "SpawnEmitterFlagBits");

    auto& wrenClass = module.klass<ParticleModule>("Particles");
    wrenClass.funcExt<bindings::LoadEmitterPresets>("LoadEmitterPresets");
    wrenClass.funcExt<bindings::SpawnEmitter>("SpawnEmitter");
    wrenClass.funcExt<bindings::SpawnBurst>("SpawnBurst");
}