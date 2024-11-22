#include "audio_module.hpp"

#include "fmod_errors.h"
#include "log.hpp"
#include <memory>

AudioModule::AudioModule()
{
}

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;

    FMOD::System* rawSystem = nullptr;
    FMOD_RESULT result = FMOD::System_Create(&rawSystem);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    system = std::unique_ptr<FMOD::System>(rawSystem);

    result = system->init(512, FMOD_INIT_NORMAL, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        system->release();
        return tickOrder;
    }

    return tickOrder;
}
void AudioModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    system->release();
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_RESULT result = system->update();

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
}
