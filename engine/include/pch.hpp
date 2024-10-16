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
#include <cereal/cereal.hpp>
#include <filesystem>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>
#include <spirv_reflect.h>

#define GLM_ENABLE_EXPERIMENTAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "vk_mem_alloc.h"

#pragma GCC diagnostic pop

#include "common.hpp"
#include "vulkan_brain.hpp"
#include "imgui.h"
#include <implot.h>
#include "tracy/Tracy.hpp"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT { 3 };
constexpr uint32_t DEFERRED_ATTACHMENT_COUNT { 4 };
