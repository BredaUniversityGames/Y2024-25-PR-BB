#pragma once

#include <entt/entity/entity.hpp>
#include <string>
#include <vector>

#include "resource_manager.hpp"

class Engine;
struct CPUModel;
struct GPUModel;

namespace SceneLoading
{
// Loads multiple models into the scene at once, using the given models data.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<CPUModel>& cpuModels);
// Loads multiple models into the scene at once, using the gpu model data.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<CPUModel>& cpuModels, const std::vector<ResourceHandle<GPUModel>>& gpuModels);
// Loads multiple models into the scene at once, using the given path to the file.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<std::string>& paths);
};
