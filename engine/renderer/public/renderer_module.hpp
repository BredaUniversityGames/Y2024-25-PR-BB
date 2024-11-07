#pragma once

#include "imgui_impl_vulkan.h"
#include "module_interface.hpp"
#include "vulkan/vulkan.hpp"

class RendererModule final : public ModuleInterface
{
public:
    RendererModule();
    ~RendererModule() final = default;

private:
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;
};
