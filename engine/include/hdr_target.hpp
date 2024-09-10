#pragma once
#include "include.hpp"

struct HDRTarget
{
    vk::Format format;
    glm::uvec2 size;
    vk::Image images;
    vk::ImageView imageViews;
    VmaAllocation allocations;
};