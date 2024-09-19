#pragma once

struct InitInfo
{
    uint32_t extensionCount{ 0 };
    const char* const* extensions{ nullptr };
    uint32_t width, height;

    std::function<vk::SurfaceKHR(vk::Instance)> retrieveSurface;
};