#pragma once

#include "constants.hpp"
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include <memory>

struct SceneDescription;
class GPUScene;
class BatchBuffer;
class ECS;
class GraphicsContext;

struct GPUSceneCreation
{
    std::shared_ptr<GraphicsContext> context;
    std::shared_ptr<ECS> ecs;

    // TODO: When we switch to ECS, fetch this data from a component in the world
    ResourceHandle<GPUImage> irradianceMap;
    ResourceHandle<GPUImage> prefilterMap;
    ResourceHandle<GPUImage> brdfLUTMap;
    ResourceHandle<GPUImage> directionalShadowMap;
};

struct RenderSceneDescription
{
    std::shared_ptr<GPUScene> gpuScene;
    std::shared_ptr<const SceneDescription> sceneDescription; // This will change to ecs
    std::shared_ptr<const ECS> ecs;
    std::shared_ptr<BatchBuffer> batchBuffer;
    uint32_t targetSwapChainImageIndex;
};

constexpr uint32_t MAX_INSTANCES = 2048;

class GPUScene
{
public:
    GPUScene(const GPUSceneCreation& creation);
    ~GPUScene();

    NON_COPYABLE(GPUScene);
    NON_MOVABLE(GPUScene);

    void Update(uint32_t frameIndex);

    const vk::DescriptorSet& GetSceneDescriptorSet(uint32_t frameIndex) const { return _sceneFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetObjectInstancesDescriptorSet(uint32_t frameIndex) const { return _objectInstancesFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return _sceneDescriptorSetLayout; }
    const vk::DescriptorSetLayout& GetObjectInstancesDescriptorSetLayout() const { return _objectInstancesDescriptorSetLayout; }

    ResourceHandle<Buffer> IndirectDrawBuffer(uint32_t frameIndex) const { return _indirectDrawFrameData[frameIndex].buffer; }
    vk::DescriptorSetLayout DrawBufferLayout() const { return _drawBufferDescriptorSetLayout; }
    vk::DescriptorSet DrawBufferDescriptorSet(uint32_t frameIndex) const { return _indirectDrawFrameData[frameIndex].descriptorSet; }

    ResourceHandle<Buffer> IndirectCountBuffer(uint32_t frameIndex) const { return _indirectDrawFrameData[frameIndex].buffer; }
    uint32_t IndirectCountOffset() const { return MAX_INSTANCES * sizeof(vk::DrawIndexedIndirectCommand); }

    uint32_t DrawCount() const { return _drawCommands.size(); };
    const std::vector<vk::DrawIndexedIndirectCommand>& DrawCommands() const { return _drawCommands; }
    uint32_t DrawCommandIndexCount() const
    {
        uint32_t count { 0 };
        for (const auto& command : _drawCommands)
        {
            count += command.indexCount;
        }
        return count;
    }

    const Camera& DirectionalLightShadowCamera() const { return _directionalLightShadowCamera; }

    ResourceHandle<GPUImage> irradianceMap;
    ResourceHandle<GPUImage> prefilterMap;
    ResourceHandle<GPUImage> brdfLUTMap;
    ResourceHandle<GPUImage> directionalShadowMap;

private:
    struct alignas(16) DirectionalLightData
    {
        glm::mat4 lightVP;
        glm::mat4 depthBiasMVP;

        glm::vec4 direction;
        glm::vec4 color;
    };

    struct alignas(16) SceneData
    {
        DirectionalLightData directionalLight;

        uint32_t irradianceIndex;
        uint32_t prefilterIndex;
        uint32_t brdfLUTIndex;
        uint32_t shadowMapIndex;
    };

    struct alignas(16) InstanceData
    {
        glm::mat4 model;

        uint32_t materialIndex;
        float boundingRadius;
    };

    struct FrameData
    {
        ResourceHandle<Buffer> buffer;
        vk::DescriptorSet descriptorSet;
    };

    std::shared_ptr<GraphicsContext> _context;
    std::shared_ptr<const ECS> _ecs;

    vk::DescriptorSetLayout _sceneDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _sceneFrameData;
    vk::DescriptorSetLayout _objectInstancesDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _objectInstancesFrameData;
    vk::DescriptorSetLayout _drawBufferDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _indirectDrawFrameData;

    std::vector<vk::DrawIndexedIndirectCommand> _drawCommands;

    // TODO: Change GPUScene to support all cameras in the scene
    Camera _directionalLightShadowCamera {};

    void UpdateSceneData(uint32_t frameIndex);
    void UpdateObjectInstancesData(uint32_t frameIndex);

    void InitializeSceneBuffers();
    void InitializeObjectInstancesBuffers();

    void CreateSceneDescriptorSetLayout();
    void CreateObjectInstanceDescriptorSetLayout();

    void CreateSceneDescriptorSets();
    void CreateObjectInstancesDescriptorSets();

    void UpdateSceneDescriptorSet(uint32_t frameIndex);
    void UpdateObjectInstancesDescriptorSet(uint32_t frameIndex);

    void CreateSceneBuffers();
    void CreateObjectInstancesBuffers();

    void InitializeIndirectDrawBuffer();
    void InitializeIndirectDrawDescriptor();

    void WriteDraws(uint32_t frameIndex);
};