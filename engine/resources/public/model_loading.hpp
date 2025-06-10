#pragma once

#include "components/animation_channel_component.hpp"
#include "cpu_resources.hpp"
#include "thread_pool.hpp"

#include <vector>

namespace ModelLoading
{
// Loads a GLTF model from the given path to the file.
NO_DISCARD CPUModel LoadGLTF(std::string_view path);
NO_DISCARD CPUModel LoadGLTFFast(ThreadPool& scheduler, std::string_view path, bool genCollision);
}
