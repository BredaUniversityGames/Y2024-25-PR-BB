#pragma once

#include "camera.hpp"
#include "constants.hpp"
#include "gpu_resources.hpp"
#include "range.hpp"
#include "resource_manager.hpp"
#include "vulkan_include.hpp"

#include <memory>

class GPUScene;
class BatchBuffer;
class ECSModule;
class GraphicsContext;

struct GPUSceneCreation
{
    std::shared_ptr<GraphicsContext> context;
    ECSModule& ecs;

    ResourceHandle<GPUImage> irradianceMap;
    ResourceHandle<GPUImage> prefilterMap;
    ResourceHandle<GPUImage> brdfLUTMap;
    ResourceHandle<GPUImage> directionalShadowMap;
};

struct RenderSceneDescription
{
    std::shared_ptr<GPUScene> gpuScene;
    ECSModule& ecs;
    std::shared_ptr<BatchBuffer> staticBatchBuffer;
    std::shared_ptr<BatchBuffer> skinnedBatchBuffer;
    uint32_t targetSwapChainImageIndex;
    float deltaTime;
};

struct DrawIndexedIndirectCommand
{
    vk::DrawIndexedIndirectCommand command;
};

constexpr uint32_t MAX_INSTANCES = 4096 * 4;
constexpr uint32_t MAX_BONES = 2048;

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

    const vk::DescriptorSetLayout GetSkinDescriptorSetLayout() const { return _skinDescriptorSetLayout; }
    const vk::DescriptorSet GetSkinDescriptorSet(uint32_t frameIndex) const { return _skinDescriptorSets[frameIndex]; }

    const Range& StaticDrawRange() const { return _staticDrawRange; }
    const Range& SkinnedDrawRange() const { return _skinnedDrawRange; }

    uint32_t DrawCount() const { return _drawCommands.size(); };
    const std::vector<DrawIndexedIndirectCommand>& DrawCommands() const { return _drawCommands; }
    uint32_t DrawCommandIndexCount() const
    {
        uint32_t count { 0 };
        for (const auto& command : _drawCommands)
        {
            count += command.command.indexCount;
        }
        return count;
    }

    const CameraResource& MainCamera() const { return _mainCamera; }
    const CameraResource& DirectionalLightShadowCamera() const { return _directionalLightShadowCamera; }

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
        uint32_t boneOffset;
    };

    struct FrameData
    {
        ResourceHandle<Buffer> buffer;
        vk::DescriptorSet descriptorSet;
    };

    std::shared_ptr<GraphicsContext> _context;
    ECSModule& _ecs;

    vk::DescriptorSetLayout _sceneDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _sceneFrameData;
    vk::DescriptorSetLayout _objectInstancesDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _objectInstancesFrameData;
    vk::DescriptorSetLayout _drawBufferDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _indirectDrawFrameData;

    std::vector<DrawIndexedIndirectCommand> _drawCommands;

    Range _staticDrawRange;
    Range _skinnedDrawRange;

    // TODO: Handle all camera's in one buffer or array to enable better culling
    CameraResource _mainCamera;
    CameraResource _directionalLightShadowCamera;

    vk::DescriptorSetLayout _skinDescriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _skinDescriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _skinBuffers;

    void UpdateSceneData(uint32_t frameIndex);
    void UpdateObjectInstancesData(uint32_t frameIndex);
    void UpdateDirectionalLightData(SceneData& scene, uint32_t frameIndex);
    void UpdateCameraData(uint32_t frameIndex);
    void UpdateSkinBuffers(uint32_t frameIndex);

    void InitializeSceneBuffers();
    void InitializeObjectInstancesBuffers();
    void InitializeSkinBuffers();

    void CreateSceneDescriptorSetLayout();
    void CreateObjectInstanceDescriptorSetLayout();
    void CreateSkinDescriptorSetLayout();

    void CreateSceneDescriptorSets();
    void CreateObjectInstancesDescriptorSets();
    void CreateSkinDescriptorSets();

    void UpdateSceneDescriptorSet(uint32_t frameIndex);
    void UpdateObjectInstancesDescriptorSet(uint32_t frameIndex);
    void UpdateSkinDescriptorSet(uint32_t frameIndex);

    void CreateSceneBuffers();
    void CreateObjectInstancesBuffers();
    void CreateSkinBuffers();

    void InitializeIndirectDrawBuffer();
    void InitializeIndirectDrawDescriptor();

    void WriteDraws(uint32_t frameIndex);
};