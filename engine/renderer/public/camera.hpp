#pragma once

#include "common.hpp"
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

    void Update(uint32_t currentFrame, ECSModule& ecs, entt::entity entity, std::optional<glm::mat4> view = std::nullopt, std::optional<glm::mat4> proj = std::nullopt);

    vk::DescriptorSet DescriptorSet(uint32_t frameIndex) const { return _descriptorSets[frameIndex]; }
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