#pragma once

#include "particle_module.hpp"

class ECSModule;

class ParticleEditor
{
public:
    ParticleEditor(ParticleModule& particleModule, ECSModule& ecsModule);
    ~ParticleEditor() = default;

    void Render();

private:
    ParticleModule& _particleModule;
    ECSModule& _ecsModule;

    std::string _selectedPresetName = "null";
    std::string _selectedPresetEditingName = "null";

    std::string _nameChangeMessage = "";
    std::string _imageLoadMessage = "Ready to load...";

    void RenderEmitterPresetList();
    void RenderEmitterPresetEditor();
};