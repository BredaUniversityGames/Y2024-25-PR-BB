#pragma once
#include <array>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class VulkanBrain;

struct Camera
{
    enum class Projection
    {
        ePerspective,
        eOrthographic
    } projection;

    glm::vec3 position {};
    glm::vec3 eulerRotation {};
    float fov {};

    float orthographicSize;

    float nearPlane {};
    float farPlane {};
    float aspectRatio {};
};

class CameraResource
{
public:
    CameraResource(const VulkanBrain& brain);
    ~CameraResource();

    void Update(uint32_t currentFrame, const Camera& camera);

    vk::DescriptorSet DescriptorSet(uint32_t frameIndex) const { return _descriptorSets[frameIndex]; }
    ResourceHandle<Buffer> BufferResource(uint32_t frameIndex) const { return _buffers[frameIndex]; }

    static vk::DescriptorSetLayout DescriptorSetLayout();

    NON_COPYABLE(CameraResource);
    NON_MOVABLE(CameraResource);

private:
    const VulkanBrain& _brain;

    static vk::DescriptorSetLayout _descriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _buffers;

    static void CreateDescriptorSetLayout(const VulkanBrain& brain);
    void CreateBuffers();
    void CreateDescriptorSets();
};