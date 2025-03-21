#pragma once

#include "common.hpp"
#include "components/camera_component.hpp"
#include "constants.hpp"
#include "gpu_resources.hpp"

#include <array>
#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

class GraphicsContext;
struct TransformComponent;
struct CameraComponent;
class ECSModule;

class CameraResource
{
public:
    CameraResource(const std::shared_ptr<GraphicsContext>& context, bool useReverseZ);
    ~CameraResource();

    NON_COPYABLE(CameraResource);
    NON_MOVABLE(CameraResource);

    void Update(uint32_t currentFrame, const CameraComponent& camera, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& position);

    static glm::mat4 CalculateProjectionMatrix(const CameraComponent& camera);
    static glm::mat4 CalculateViewMatrix(const glm::quat& rotation, const glm::vec3& position);

    vk::DescriptorSet DescriptorSet(uint32_t frameIndex) const
    {
        return _descriptorSets[frameIndex];
    }
    ResourceHandle<Buffer> BufferResource(uint32_t frameIndex) const { return _buffers[frameIndex]; }

    bool UsesReverseZ() const { return _useReverseZ; }

    static vk::DescriptorSetLayout DescriptorSetLayout();

private:
    std::shared_ptr<GraphicsContext> _context;

    static vk::DescriptorSetLayout _descriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _buffers;

    bool _useReverseZ;

    static void CreateDescriptorSetLayout(const std::shared_ptr<GraphicsContext>& context);
    void CreateBuffers();
    void CreateDescriptorSets();
};