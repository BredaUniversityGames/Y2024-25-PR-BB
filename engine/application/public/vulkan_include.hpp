#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#include <vulkan/vulkan.hpp>

#pragma clang diagnostic pop

// Undefining problematic X11 defines

#undef Bool
#undef None
#undef Convex