#include "audio_system.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "fmod_debug.hpp"

#include <fmod.h>
#include <ranges>

#include <tracy/Tracy.hpp>

AudioSystem::AudioSystem(ECSModule& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECSModule& ecs, MAYBE_UNUSED float dt)
{
    const auto& listenerView = ecs.GetRegistry().view<AudioListenerComponent>();

    if (!listenerView.empty())
    {
        const auto entity = listenerView.front();

        glm::vec3 position {};
        glm::vec3 velocity {};
        glm::vec3 forward {};
        glm::vec3 up {};

        if (ecs.GetRegistry().all_of<TransformComponent>(entity))
        {
            position = TransformHelpers::GetWorldPosition(ecs.GetRegistry(), entity);

            auto rotation = TransformHelpers::GetWorldRotation(ecs.GetRegistry(), entity);

            forward = glm::normalize(rotation * glm::vec3(0.0f, -1.0f, 0.0f));
            up = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, 1.0f));
        }

        if (ecs.GetRegistry().all_of<RigidbodyComponent>(entity))
        {
            const auto& body = ecs.GetRegistry().get<RigidbodyComponent>(entity);
            velocity = ToGLMVec3(_audioModule._physics->bodyInterface->GetLinearVelocity(body.bodyID));
        }

        _audioModule.SetListener3DAttributes(position, velocity, forward, up);
    }

    const auto& emitterView = ecs.GetRegistry().view<AudioEmitterComponent>();
    for (const auto entity : emitterView)
    {
        AudioEmitterComponent& emitter = ecs.GetRegistry().get<AudioEmitterComponent>(entity);

        // Remove sounds and events from the emitter if they are no longer playing
        std::erase_if(emitter._soundIds, [&](const auto& id)
            { return !_audioModule.IsSFXPlaying(id); });
        std::erase_if(emitter._eventIds, [&](const auto& id)
            { return !_audioModule.IsEventPlaying(id); });

        if (!ecs.GetRegistry().all_of<TransformComponent>(entity))
        {
            for (auto& eventInstance : emitter._eventIds)
            {
                if (eventInstance.startDuringSystemUpdate)
                {
                    _audioModule.StartEventInstance(eventInstance);
                    eventInstance.startDuringSystemUpdate = false;
                }
            }
            return;
        }

        // Get 3d attributes of emitter
        const glm::vec3 position = TransformHelpers::GetWorldPosition(ecs.GetRegistry(), entity);
        glm::vec3 velocity = glm::vec3(0.f, 0.f, 0.f);

        const glm::quat rotation = TransformHelpers::GetWorldRotation(ecs.GetRegistry(), entity);
        const glm::vec3 forward = glm::normalize(rotation * glm::vec3(0.f, -1.f, 0.f));
        const glm::vec3 up = glm::normalize(rotation * glm::vec3(0.f, 0.f, 1.f));

        if (RigidbodyComponent* rigidBody = ecs.GetRegistry().try_get<RigidbodyComponent>(entity))
        {
            velocity = ToGLMVec3(_audioModule._physics->bodyInterface->GetLinearVelocity(rigidBody->bodyID));
        }
        // Update 3D position of sounds
        for (auto& soundInstance : emitter._soundIds)
        {
            if (soundInstance.is3D)
            {
                _audioModule.UpdateSound3DAttributes(soundInstance.id, position, velocity);
                _audioModule.AddDebugLine(position + glm::vec3(-1.f, 1.f, -1.f), position + glm::vec3(-1.f, 1.f, 1.f));
                _audioModule.AddDebugLine(position + glm::vec3(1.f, 1.f, -1.f), position + glm::vec3(1.f, 1.f, 1.f));
                _audioModule.AddDebugLine(position + glm::vec3(-1.f, -1.f, -1.f), position + glm::vec3(-1.f, -1.f, 1.f));
                _audioModule.AddDebugLine(position + glm::vec3(1.f, -1.f, -1.f), position + glm::vec3(1.f, -1.f, 1.f));
            }
        }
        // Update 3D position of events
        for (auto& eventInstance : emitter._eventIds)
        {
            _audioModule.SetEvent3DAttributes(eventInstance, position, velocity, forward, up);
            _audioModule.AddDebugLine(position + glm::vec3(-1.f, 1.f, -1.f), position + glm::vec3(-1.f, 1.f, 1.f));
            _audioModule.AddDebugLine(position + glm::vec3(1.f, 1.f, -1.f), position + glm::vec3(1.f, 1.f, 1.f));
            _audioModule.AddDebugLine(position + glm::vec3(-1.f, -1.f, -1.f), position + glm::vec3(-1.f, -1.f, 1.f));
            _audioModule.AddDebugLine(position + glm::vec3(1.f, -1.f, -1.f), position + glm::vec3(1.f, -1.f, 1.f));

            if (eventInstance.startDuringSystemUpdate)
            {
                _audioModule.StartEventInstance(eventInstance);
                eventInstance.startDuringSystemUpdate = false;
            }
        }
    }
}
void AudioSystem::Inspect()
{
    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("AudioSystem", nullptr, ImGuiWindowFlags_NoResize);

    if (ImGui::TreeNode((std::string("Sounds loaded: ") + std::to_string(_audioModule._sounds.size())).c_str()))
    {
        for (const auto& pair : _audioModule._soundInfos)
        {
            ImGui::Text("--| %s", pair.second.path.data());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Sounds playing: ") + std::to_string(_audioModule._channelsActive.size())).c_str()))
    {
        for (const auto& pair : _audioModule._channelsActive)
        {
            ImGui::Text("--| %u", static_cast<uint32_t>(pair.first));
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Banks Loaded: ") + std::to_string(_audioModule._banks.size())).c_str()))
    {
        for (const auto& pair : _audioModule._banks)
        {
            ImGui::Text("--| %u", static_cast<uint32_t>(pair.first));
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode((std::string("Events Playing: ") + std::to_string(_audioModule._events.size())).c_str()))
    {
        for (const auto& pair : _audioModule._events)
        {
            ImGui::Text("--| %u", static_cast<uint32_t>(pair.first));
        }
        ImGui::TreePop();
    }

    static constexpr int spectrumSize = 128;
    static std::array<float, spectrumSize> spectrum;

    void* data = nullptr;
    unsigned int length = 0;

    FMOD_CHECKRESULT(FMOD_DSP_GetParameterData(_audioModule._fftDSP, FMOD_DSP_FFT_SPECTRUMDATA, &data, &length, nullptr, 0));

    if (const auto fftData = static_cast<FMOD_DSP_PARAMETER_FFT*>(data); fftData && fftData->numchannels > 0)
    {
        memcpy(spectrum.data(), fftData->spectrum[0], spectrumSize * sizeof(float));
    }

    ImGui::PlotLines("##Spectrum", spectrum.data(), spectrumSize, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 100));

    ImGui::End();
}