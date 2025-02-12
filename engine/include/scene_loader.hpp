#pragma once

#include <vector>
#include <string>

class Engine;

namespace SceneLoading
{
// Loads multiple models into the scene at once, using the given models data.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>>& models);
// Loads multiple models into the scene at once, using the given path to the file.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<std::string>& paths);
};
