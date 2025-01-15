#include "particle_editor.hpp"

#include "particle_module.hpp"

ParticleEditor::ParticleEditor(ParticleModule& particleModule, ECSModule& ecsModule)
    : _particleModule(particleModule)
    , _ecsModule(ecsModule)
{
}

void ParticleEditor::Render()
{
    ZoneScoped;

    ImGui::SetNextWindowSize({ 0.f, 0.f });

    ImGui::Begin("Particle editor", nullptr, NULL);

    if (ImGui::BeginChild("list", { 170, 220 }, true))
    {
        RenderEmitterPresetList();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("editor", { 300, 220 }, true))
    {
        RenderEmitterPresetEditor();
    }
    ImGui::EndChild();

    ImGui::End();
}

void ParticleEditor::RenderEmitterPresetList()
{
    ImGui::Text("Emitter Presets:");
    ImGui::SameLine();
    if (ImGui::Button("+ new"))
    {
        ParticleModule::EmitterPreset newPreset;
        newPreset.name += " " + std::to_string(_particleModule._emitterPresets.size());
        _particleModule._emitterPresets.emplace_back(std::move(newPreset));
    }

    for (uint32_t i = 0; i < _particleModule._emitterPresets.size(); i++)
    {
        auto& emitterPreset = _particleModule._emitterPresets[i];
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        ImGui::TreeNodeEx(emitterPreset.name.c_str(), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            _selectedPresetIndex = i;
            _imageLoadMessage = "Ready to load...";
        }
    }
}

void ParticleEditor::RenderEmitterPresetEditor()
{
    if (_selectedPresetIndex > -1)
    {
        auto& selectedPreset = _particleModule._emitterPresets[_selectedPresetIndex];

        ImGui::InputText("Name##EmitterPresetEditor", &selectedPreset.name);

        ImGui::DragFloat("Emit delay##EmitterPresetEditor", &selectedPreset.emitDelay, 0.0f, 50.0f);

        const char* types[] = { "Billboard", "Ribbon" };
        static const char* currentType = types[0];
        if (ImGui::BeginCombo("Type##EmitterPresetEditor", currentType))
        {
            for (int n = 0; n < IM_ARRAYSIZE(types); n++)
            {
                bool is_selected = (currentType == types[n]);
                if (ImGui::Selectable(types[n], is_selected))
                {
                    currentType = types[n];
                    selectedPreset.type = static_cast<ParticleType>(n);
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus(); // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
            }
            ImGui::EndCombo();
        }

        int emitterCount = static_cast<int>(selectedPreset.count);
        ImGui::DragInt("Count##EmitterPresetEditor", &emitterCount, 0, 1024);
        selectedPreset.count = emitterCount;
        ImGui::DragFloat("Mass##EmitterPresetEditor", &selectedPreset.mass, -100.0f, 100.0f);
        ImGui::DragFloat2("Rotation velocity##EmitterPresetEditor", &selectedPreset.rotationVelocity.x);
        ImGui::DragFloat("Max life##EmitterPresetEditor", &selectedPreset.maxLife, 0.0f, 100.0f);

        ImGui::Text("assets/textures/");
        ImGui::SameLine();
        ImGui::InputText("Image##EmitterPresetEditor", &_currentImage);

        if (ImGui::Button("Load Image##EmitterPresetEditor"))
        {
            auto image = _particleModule.GetEmitterImage(_currentImage);
            _particleModule.SetEmitterPresetImage(selectedPreset, image);
            _imageLoadMessage = "Loaded successfully!";
        }
        ImGui::SameLine();
        ImGui::Text("%s", _imageLoadMessage.c_str());

        if (ImGui::Button("Spawn##EmitterPresetEditor"))
        {
            entt::entity entity = _ecsModule.GetRegistry().create();
            NameComponent node;
            node.name = "Particle Emitter";
            _ecsModule.GetRegistry().emplace<NameComponent>(entity, node);

            _particleModule.SpawnEmitter(entity, _selectedPresetIndex, SpawnEmitterFlagBits::eIsActive | SpawnEmitterFlagBits::eSetCustomPosition | SpawnEmitterFlagBits::eSetCustomVelocity, { 8.0f, 35.0f, 300.0f });
        }

        ImGui::SameLine();

        // red button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
        if (ImGui::Button("Delete##EmitterPresetEditor"))
        {
            _particleModule._emitterPresets.erase(_particleModule._emitterPresets.begin() + _selectedPresetIndex);
            _selectedPresetIndex = NULL;
            ImGui::PopStyleColor(3);
            return;
        }
        ImGui::PopStyleColor(3);
    }
}
