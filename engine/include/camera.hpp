#pragma once

#include "include.hpp"

struct Camera
{
    glm::vec3 position{};
    glm::vec3 euler_rotation{};
    float fov{};

    float nearPlane{};
    float farPlane{};
};

struct CameraUBO
{
    alignas(16)
    glm::mat4 VP;
    glm::mat4 view;
    glm::mat4 proj;

    glm::mat4 lightVP;
    glm::mat4 depthBiasMVP;
    glm::vec4 lightData;
    alignas(16)
    glm::vec3 cameraPosition;
    float _padding;

};

struct CameraStructure
{
    vk::DescriptorSetLayout descriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> buffers;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> allocations;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> mappedPtrs;
};
