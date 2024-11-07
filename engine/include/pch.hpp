#pragma once

#include <algorithm>
#include <cereal/cereal.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spirv_reflect.h>
#include <stb_image.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "vk_mem_alloc.h"

#pragma GCC diagnostic pop

#include "common.hpp"
#include "imgui.h"
#include "tracy/Tracy.hpp"
#include "vulkan_brain.hpp"
#include <implot.h>
