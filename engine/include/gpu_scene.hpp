#pragma once

#include "vulkan_brain.hpp"
#include "constants.hpp"

struct SceneDescription;
class GPUScene;
class BatchBuffer;

struct GPUSceneCreation
{
    const VulkanBrain& brain;

    // TODO: When we switch to ECS, fetch this data from a component in the world
    ResourceHandle<Image> irradianceMap;
    ResourceHandle<Image> prefilterMap;
    ResourceHandle<Image> brdfLUTMap;
    ResourceHandle<Image> directionalShadowMap;
};

struct RenderSceneDescription
{
    const GPUScene& gpuScene;
    const SceneDescription& sceneDescription; // This will change to ecs
    const BatchBuffer& batchBuffer;
    const uint32_t targetSwapChainImageIndex;
};

constexpr uint32_t MAX_INSTANCES = 2048;

class GPUScene
{
public:
    GPUScene(const GPUSceneCreation& creation);
    ~GPUScene();

    void Update(const SceneDescription& scene, uint32_t frameIndex);

    const vk::DescriptorSet& GetSceneDescriptorSet(uint32_t frameIndex) const { return _sceneFrameData[frameIndex].descriptorSet; }
    const vk::DescriptorSet& GetObjectInstancesDescriptorSet(uint32_t frameIndex) const { return _objectInstancesFrameData[frameIndex].descriptorSet; }
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

    ResourceHandle<Image> irradianceMap;
    ResourceHandle<Image> prefilterMap;
    ResourceHandle<Image> brdfLUTMap;
    ResourceHandle<Image> directionalShadowMap;

private:
    struct alignas(16) DirectionalLightData
    {
        glm::mat4 lightVP;
        glm::mat4 depthBiasMVP;

        glm::vec4 direction;
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

    const VulkanBrain& _brain;

    vk::DescriptorSetLayout _sceneDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _sceneFrameData;
    vk::DescriptorSetLayout _objectInstancesDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _objectInstancesFrameData;
    vk::DescriptorSetLayout _drawBufferDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _indirectDrawFrameData;

    std::vector<vk::DrawIndexedIndirectCommand> _drawCommands;

    void UpdateSceneData(const SceneDescription& scene, uint32_t frameIndex);
    void UpdateObjectInstancesData(const SceneDescription& scene, uint32_t frameIndex);

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