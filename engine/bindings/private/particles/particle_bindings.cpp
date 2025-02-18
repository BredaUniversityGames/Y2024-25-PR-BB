#include "particle_bindings.hpp"

#include "entity/wren_entity.hpp"
#include "particle_module.hpp"
#include "utility/enum_bind.hpp"

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
}

void BindParticleAPI(wren::ForeignModule& module)
{
    bindings::BindEnum<EmitterPresetID>(module, "EmitterPresetID");
    bindings::BindBitflagEnum<SpawnEmitterFlagBits>(module, "SpawnEmitterFlagBits");

    auto& wren_class = module.klass<ParticleModule>("Particles");
    wren_class.funcExt<bindings::LoadEmitterPresets>("LoadEmitterPresets");
    wren_class.funcExt<bindings::SpawnEmitter>("SpawnEmitter");
}