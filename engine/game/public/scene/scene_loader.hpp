#pragma once

#include "resource_management/model_resource_manager.hpp"

#include <entt/entity/entity.hpp>
#include <string>

class Engine;

struct SceneData
{
    CPUModel cpuModel {};
    ResourceHandle<GPUModel> gpuModel {};

    entt::entity Instantiate(Engine& engine);
};

class SceneLoader
{
public:
    SceneLoader() = default;
    NON_MOVABLE(SceneLoader);
    NON_COPYABLE(SceneLoader);

    std::shared_ptr<SceneData> LoadModel(Engine& engine, std::string_view path);
    void Clear() { _models.clear(); }

private:
    std::unordered_map<std::string, std::shared_ptr<SceneData>> _models {};
};
;
