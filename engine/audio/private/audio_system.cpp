#include "audio_system.hpp"

#include "glm/glm.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

AudioSystem::AudioSystem(ECSModule& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECSModule& ecs, float dt)
{
    {
        const auto& view = ecs.GetRegistry().view<AudioListenerComponent>();

        if (!view.empty())
        {
            const auto listener = view.front();
            auto* transform = ecs.GetRegistry().try_get<TransformComponent>(listener);
            if (transform)
            {
                _audioModule.SetListener3DAttributes(TransformHelpers::GetWorldPosition(ecs.GetRegistry(), listener));
            }
        }
    }

    {
        const auto& view = ecs.GetRegistry().view<AudioEmitterComponent, TransformComponent>();
        for (const auto e : view)
        {
            AudioEmitterComponent& emitter = ecs.GetRegistry().get<AudioEmitterComponent>(e);
            TransformComponent& transform = ecs.GetRegistry().get<TransformComponent>(e);

            for (auto id : emitter.ids)
            {
            }
        }
    }
}
void AudioSystem::Inspect()
{
    ImGui::Begin("AudioSystem");

    ImGui::Text("Sounds loaded: %u", _audioModule._sounds.size());
    // if (ImGui::TreeNode("Collapsing Headers"))
    // {
    //     static bool closable_group = true;
    //     ImGui::Checkbox("Show 2nd header", &closable_group);
    //     if (ImGui::CollapsingHeader("Header", ImGuiTreeNodeFlags_None))
    //     {
    //         for ()
    //     }
    //     ImGui::TreePop();
    // }
    ImGui::Text("Sounds playing: %u", _audioModule._channelsActive.size());
    ImGui::Text("Banks loaded: %u", _audioModule._banks.size());
    ImGui::Text("Events playing %u", _audioModule._events.size());

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