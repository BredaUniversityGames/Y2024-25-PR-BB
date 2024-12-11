#include "audio_system.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "fmod_debug.hpp"

#include <fmod.h>
#include <ranges>

AudioSystem::AudioSystem(ECSModule& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECSModule& ecs, float dt)
{
    const auto& listenerView = ecs.GetRegistry().view<AudioListenerComponent>();

    if (!listenerView.empty())
    {
        const auto entity = listenerView.front();
        if (ecs.GetRegistry().all_of<TransformComponent>(entity))
        {
            _audioModule.SetListener3DAttributes(TransformHelpers::GetWorldPosition(ecs.GetRegistry(), entity));
        }
    }

    const auto& emitterView = ecs.GetRegistry().view<AudioEmitterComponent, TransformComponent>();
    for (const auto entity : emitterView)
    {
        AudioEmitterComponent& emitter = ecs.GetRegistry().get<AudioEmitterComponent>(entity);

        for (auto soundInstance : emitter.ids)
        {
            if (soundInstance.is3D)
            {
                _audioModule.Update3DSoundPosition(soundInstance.id, TransformHelpers::GetWorldPosition(ecs.GetRegistry(), entity));
            }
        }
    }
}
void AudioSystem::Inspect()
{
    ImGui::Begin("AudioSystem");

    if (ImGui::TreeNode((std::string("Sounds loaded: ") + std::to_string(_audioModule._sounds.size())).c_str()))
    {
        for (const auto snd : _audioModule._soundInfos | std::views::values)
        {
            ImGui::Text("--| %s", snd->path.data());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Sounds playing: ") + std::to_string(_audioModule._channelsActive.size())).c_str()))
    {
        for (const auto key : _audioModule._channelsActive | std::views::keys)
        {
            ImGui::Text("--| %u", key);
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Banks Loaded: ") + std::to_string(_audioModule._banks.size())).c_str()))
    {
        for (const auto key : _audioModule._banks | std::views::keys)
        {
            ImGui::Text("--| %u", key);
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Events Playing: ") + std::to_string(_audioModule._events.size())).c_str()))
    {
        for (const auto key : _audioModule._events | std::views::keys)
        {
            ImGui::Text("--| %u", key);
        }
        ImGui::TreePop();
    }

    static constexpr int spectrumSize = 128;
    static float spectrum[spectrumSize];

    void* data;
    unsigned int length = 0;

    FMOD_CHECKRESULT(FMOD_DSP_GetParameterData(_audioModule._fftDSP, FMOD_DSP_FFT_SPECTRUMDATA, &data, &length, nullptr, NULL));

    if (const auto fftData = static_cast<FMOD_DSP_PARAMETER_FFT*>(data); fftData && fftData->numchannels > 0)
    {
        memcpy(spectrum, fftData->spectrum[0], spectrumSize * sizeof(float));
    }

    ImGui::PlotLines("##Spectrum", spectrum, spectrumSize, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 100));

    ImGui::End();
}