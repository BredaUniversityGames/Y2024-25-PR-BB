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

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "vk_mem_alloc.h"

#pragma GCC diagnostic pop

#include "common.hpp"
#include "vulkan_brain.hpp"
#include "imgui.h"
#include <implot.h>
#include "tracy/Tracy.hpp"
