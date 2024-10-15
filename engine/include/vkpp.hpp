#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

// Problematic definitions that can come from X11

#undef None
#undef Bool
#undef Convex