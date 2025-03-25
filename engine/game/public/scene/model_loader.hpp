#pragma once

#include "resource_management/model_resource_manager.hpp"

#include <entt/entity/entity.hpp>
#include <string>

class Engine;

struct ModelData
{
    ModelData(const CPUModel& cpu, ResourceHandle<GPUModel> gpu)
        : cpuModel(cpu)
        , gpuModel(gpu)
    {
    }

    CPUModel cpuModel {};
    ResourceHandle<GPUModel> gpuModel {};

    entt::entity Instantiate(Engine& engine);
};

class ModelLoader
{
public:
    ModelLoader() = default;
    NON_MOVABLE(ModelLoader);
    NON_COPYABLE(ModelLoader);

    std::shared_ptr<ModelData> LoadModel(Engine& engine, std::string_view path);
    void Clear() { _models.clear(); }

private:
    std::unordered_map<std::string, std::shared_ptr<ModelData>> _models {};
};
;
