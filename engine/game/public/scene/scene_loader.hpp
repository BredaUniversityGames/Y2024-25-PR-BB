#pragma once

#include <entt/entity/entity.hpp>
#include <string>

class Engine;

namespace SceneLoading
{
entt::entity LoadModel(Engine& engine, const std::string& path);
};
