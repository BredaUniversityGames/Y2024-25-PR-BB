#include "model_loading_module.hpp"
#include "model_loader.hpp"

ModuleTickOrder ModelLoadingModule::Init(MAYBE_UNUSED Engine& engine)
{
    _loader = std::make_unique<ModelLoader>();
    return ModuleTickOrder::eTick;
}

CPUModel ModelLoadingModule::LoadGLTF(std::string_view path)
{
    return _loader->LoadGLTF(path);
}

void ModelLoadingModule::ReadGeometrySizeGLTF(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize)
{
    return _loader->ReadGeometrySizeGLTF(path, vertexBufferSize, indexBufferSize);
}
