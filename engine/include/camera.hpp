#pragma once
#include <array>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera
{
    glm::vec3 position {};
    glm::vec3 euler_rotation {};
    float fov {};

    float nearPlane {};
    float farPlane {};
};

struct alignas(16) CameraUBO
{
    glm::mat4 VP;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 skydomeMVP;

    glm::vec3 cameraPosition;
};

struct CameraStructure
{
    vk::DescriptorSetLayout descriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> buffers;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> allocations;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> mappedPtrs;
};
