#pragma once

class ParticleModule;

class ParticleEditor
{
public:
    ParticleEditor(ParticleModule& particleModule, ECSModule& ecsModule);
    ~ParticleEditor() = default;

    void Render();

private:
    ParticleModule& _particleModule;
    ECSModule& _ecsModule;

    int _selectedPresetIndex = -1;

    void RenderEmitterPresetList();
    void RenderEmitterPresetEditor();
};