#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include <set>
#include <thread>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "vk_mem_alloc.h"

#pragma GCC diagnostic pop

#include "class_decorations.hpp"
#include "vulkan_brain.hpp"
#include "imgui.h"
#include <implot.h>
#include "tracy/Tracy.hpp"

// inline void* operator new(std::size_t count)
// {
//     auto ptr = malloc(count);
//     TracyAlloc(ptr, count);
//     return ptr;
// }
// inline void operator delete(void* ptr) noexcept
// {
//     TracyFree(ptr);
//     free(ptr);
// }

constexpr uint32_t MAX_FRAMES_IN_FLIGHT { 3 };
constexpr uint32_t DEFERRED_ATTACHMENT_COUNT { 4 };

#ifdef _WIN32
#define WINDOWS
#elif __linux__
#define LINUX
#endif
