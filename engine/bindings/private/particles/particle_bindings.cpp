#include "particle_bindings.hpp"

#include "particle_module.hpp"
#include "utility/enum_bind.hpp"
#include "utility/wren_entity.hpp"

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

class ParticleFlagUtil
{
public:
    static uint8_t GetSpawnEmitterFlagBits(SpawnEmitterFlagBits flag)
    {
        return static_cast<uint8_t>(flag);
    }
};

void BindParticleAPI(wren::ForeignModule& module)
{
    bindings::BindEnum<EmitterPresetID>(module, "EmitterPresetID");
    bindings::BindEnum<SpawnEmitterFlagBits>(module, "SpawnEmitterFlagBits");

    auto& wren_class = module.klass<ParticleModule>("Particles");
    wren_class.funcExt<bindings::LoadEmitterPresets>("LoadEmitterPresets");
    wren_class.funcExt<bindings::SpawnEmitter>("SpawnEmitter");

    auto& particleUtilClass = module.klass<ParticleFlagUtil>("ParticleFlagUtil");
    particleUtilClass.funcStatic<&ParticleFlagUtil::GetSpawnEmitterFlagBits>("GetSpawnEmitterFlagBits");
}