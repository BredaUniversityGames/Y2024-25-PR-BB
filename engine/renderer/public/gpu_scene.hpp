#pragma once

#include "camera.hpp"
#include "constants.hpp"
#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "vulkan_include.hpp"

#include <memory>
#include <tracy/TracyVulkan.hpp>

class GPUScene;
class BatchBuffer;
class ECSModule;
class GraphicsContext;
class CameraBatch;

struct GPUSceneCreation
{
    std::shared_ptr<GraphicsContext> context;
    ECSModule& ecs;

    ResourceHandle<GPUImage> irradianceMap;
    ResourceHandle<GPUImage> prefilterMap;
    ResourceHandle<GPUImage> brdfLUTMap;
    ResourceHandle<GPUImage> depthImage;

    glm::uvec2 displaySize;
};

struct RenderSceneDescription
{
    std::shared_ptr<GPUScene> gpuScene;
    ECSModule& ecs;
    std::shared_ptr<BatchBuffer> staticBatchBuffer;
    std::shared_ptr<BatchBuffer> skinnedBatchBuffer;
    uint32_t targetSwapChainImageIndex;
    float deltaTime;
    TracyVkCtx& tracyContext;
};

constexpr uint32_t MAX_STATIC_INSTANCES = 1 << 14;
constexpr uint32_t MAX_SKINNED_INSTANCES = 1 << 10;
constexpr uint32_t MAX_POINT_LIGHTS = 1 << 13;
constexpr uint32_t MAX_BONES = 1 << 11;

struct DrawIndexedIndirectCommand
{
    vk::DrawIndexedIndirectCommand command;
};

class GPUScene
{
public:
    GPUScene(const GPUSceneCreation& creation, const Settings::Fog& settings);
    ~GPUScene();

    NON_COPYABLE(GPUScene);
    NON_MOVABLE(GPUScene);

    void Update(uint32_t frameIndex);

    const vk::DescriptorSet& GetSceneDescriptorSet(uint32_t frameIndex) const { return _sceneFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetStaticInstancesDescriptorSet(uint32_t frameIndex) const { return _staticInstancesFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetSkinnedInstancesDescriptorSet(uint32_t frameIndex) const { return _skinnedInstancesFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetPointLightDescriptorSet(uint32_t frameIndex) const { return _pointLightFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return _sceneDescriptorSetLayout; }
    const vk::DescriptorSetLayout& GetObjectInstancesDescriptorSetLayout() const { return _objectInstancesDSL; }
    const vk::DescriptorSetLayout& GetPointLightDescriptorSetLayout() const { return _pointLightDSL; }
    const vk::DescriptorSetLayout& GetHZBDescriptorSetLayout() const { return _hzbImageDSL; }

    vk::DescriptorSetLayout DrawBufferLayout() const { return _drawBufferDSL; }
    ResourceHandle<Buffer> StaticDrawBuffer(uint32_t frameIndex) const { return _staticDraws[frameIndex].buffer; }
    vk::DescriptorSet StaticDrawDescriptorSet(uint32_t frameIndex) const { return _staticDraws[frameIndex].descriptorSet; }
    ResourceHandle<Buffer> SkinnedDrawBuffer(uint32_t frameIndex) const { return _skinnedDraws[frameIndex].buffer; }
    vk::DescriptorSet SkinnedDrawDescriptorSet(uint32_t frameIndex) const { return _skinnedDraws[frameIndex].descriptorSet; }

    const vk::DescriptorSetLayout GetSkinDescriptorSetLayout() const { return _skinDescriptorSetLayout; }
    const vk::DescriptorSet GetSkinDescriptorSet(uint32_t frameIndex) const { return _skinDescriptorSets[frameIndex]; }

    uint32_t StaticDrawCount() const { return _staticDrawCommands.size(); };
    const std::vector<DrawIndexedIndirectCommand>& StaticDrawCommands() const { return _staticDrawCommands; }

    uint32_t SkinnedDrawCount() const { return _skinnedDrawCommands.size(); };
    const std::vector<DrawIndexedIndirectCommand>& SkinnedDrawCommands() const { return _skinnedDrawCommands; }
    uint32_t DrawCommandIndexCount(std::vector<DrawIndexedIndirectCommand> commands) const
    {
        uint32_t count { 0 };
        for (size_t i = 0; i < commands.size(); ++i)
        {
            const auto& command = commands[i];
            count += command.command.indexCount;
        }
        return count;
    }

    const CameraResource& MainCamera() const { return _mainCamera; }
    CameraBatch& MainCameraBatch() const { return *_mainCameraBatch; }

    ResourceHandle<GPUImage> Shadow() const { return _shadowImage; }

    const CameraResource& DirectionalLightShadowCamera() const { return _directionalLightShadowCamera; }
    CameraBatch& ShadowCameraBatch() const { return *_shadowCameraBatch; }

    ResourceHandle<GPUImage> irradianceMap;
    ResourceHandle<GPUImage> prefilterMap;
    ResourceHandle<GPUImage> brdfLUTMap;

private:
    struct alignas(16) DirectionalLightData
    {
        glm::mat4 lightVP;
        glm::mat4 depthBiasMVP;

        glm::vec4 direction;
        glm::vec4 color;
    };

    struct alignas(16) PointLightData
    {
        glm::vec3 position;
        float range;
        glm::vec3 color;
        float attenuation;
    };

    struct alignas(16) PointLightArray
    {
        std::array<PointLightData, MAX_POINT_LIGHTS> lights;
        uint32_t count;
    };

    struct alignas(16) SceneData
    {
        DirectionalLightData directionalLight;

        uint32_t irradianceIndex;
        uint32_t prefilterIndex;
        uint32_t brdfLUTIndex;
        uint32_t shadowMapIndex;

        glm::vec3 fogColor;
        float fogDensity;
        float fogHeight;
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

    struct PointLightFrameData
    {
        ResourceHandle<Buffer> buffer;
        vk::DescriptorSet descriptorSet;
    };

    std::shared_ptr<GraphicsContext> _context;
    const Settings::Fog& _settings;
    ECSModule& _ecs;

    vk::DescriptorSetLayout _sceneDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _sceneFrameData;
    vk::DescriptorSetLayout _objectInstancesDSL;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _staticInstancesFrameData;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _skinnedInstancesFrameData;
    vk::DescriptorSetLayout _drawBufferDSL;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _staticDraws;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _skinnedDraws;
    vk::DescriptorSetLayout _pointLightDSL;
    std::array<PointLightFrameData, MAX_FRAMES_IN_FLIGHT> _pointLightFrameData;

    std::vector<DrawIndexedIndirectCommand> _staticDrawCommands;
    std::vector<DrawIndexedIndirectCommand> _skinnedDrawCommands;

    CameraResource _mainCamera;
    CameraResource _directionalLightShadowCamera;

    std::unique_ptr<CameraBatch> _mainCameraBatch;
    std::unique_ptr<CameraBatch> _shadowCameraBatch;

    ResourceHandle<GPUImage> _shadowImage;
    ResourceHandle<Sampler> _shadowSampler;

    vk::DescriptorSetLayout _skinDescriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _skinDescriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _skinBuffers;

    vk::DescriptorSetLayout _visibilityDSL;
    vk::DescriptorSetLayout _redirectDSL;
    vk::DescriptorSetLayout _hzbImageDSL;

    void UpdateSceneData(uint32_t frameIndex);
    void UpdatePointLightArray(uint32_t frameIndex);
    void UpdateObjectInstancesData(uint32_t frameIndex);
    void UpdateDirectionalLightData(SceneData& scene, uint32_t frameIndex);
    void UpdatePointLightData(PointLightArray& pointLightArray, uint32_t frameIndex);
    void UpdateCameraData(uint32_t frameIndex);
    void UpdateSkinBuffers(uint32_t frameIndex);

    void InitializeSceneBuffers();
    void InitializePointLightBuffer();
    void InitializeObjectInstancesBuffers();
    void InitializeSkinBuffers();

    void CreateSceneDescriptorSetLayout();
    void CreatePointLightDescriptorSetLayout();
    void CreateObjectInstanceDescriptorSetLayout();
    void CreateSkinDescriptorSetLayout();
    void CreateHZBDescriptorSetLayout();

    void CreateSceneDescriptorSets();
    void CreatePointLightDescriptorSets();
    void CreateObjectInstancesDescriptorSets();
    void CreateSkinDescriptorSets();

    void UpdateSceneDescriptorSet(uint32_t frameIndex);
    void UpdatePointLightDescriptorSet(uint32_t frameIndex);
    void UpdateObjectInstancesDescriptorSet(uint32_t frameIndex);
    void UpdateSkinDescriptorSet(uint32_t frameIndex);

    void CreateSceneBuffers();
    void CreatePointLightBuffer();
    void CreateObjectInstancesBuffers();
    void CreateSkinBuffers();

    void InitializeIndirectDrawBuffer();
    void InitializeIndirectDrawDescriptor();

    void WriteDraws(uint32_t frameIndex);

    void CreateShadowMapResources();
};