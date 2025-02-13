#pragma once
#include "engine.hpp"
#include "cpu_resources.hpp"
#include "model_loader.hpp"
#include <memory>

class ModelLoader;

class ModelLoadingModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override {}
    void Shutdown(MAYBE_UNUSED Engine& engine) override {}

    std::string_view GetName() override { return "Model Loading Module"; }

    std::unique_ptr<ModelLoader> _loader;

public:
    NON_COPYABLE(ModelLoadingModule);
    NON_MOVABLE(ModelLoadingModule);

    ModelLoadingModule() = default;
    ~ModelLoadingModule() override = default;

    NO_DISCARD CPUModel LoadGLTF(std::string_view path);
    void ReadGeometrySizeGLTF(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);
};
