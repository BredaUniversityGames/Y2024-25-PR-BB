#pragma once

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
    bool distanceCullingEnabled;
    float frustum[4];
    float zNear;
    float zFar;
    bool cullingEnabled;

    glm::vec3 _padding {};
};

struct CameraStructure
{
    vk::DescriptorSetLayout descriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> buffers;
};
