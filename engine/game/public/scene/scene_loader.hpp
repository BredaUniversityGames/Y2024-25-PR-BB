#pragma once

#include <vector>
#include <string>
#include <entt/entity/entity.hpp>

class Engine;
struct CPUModel;

namespace SceneLoading
{
// Loads multiple models into the scene at once, using the given models data.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<CPUModel>& cpuModels);
// Loads multiple models into the scene at once, using the given path to the file.
std::vector<entt::entity> LoadModels(Engine& engine, const std::vector<std::string>& paths);
};
