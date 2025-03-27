#pragma once

#include "camera.hpp"
#include "constants.hpp"
#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "resources/sampler.hpp"
#include "settings.hpp"

#include <entt/entity/entity.hpp>
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
constexpr uint32_t MAX_SKINNED_INSTANCES = 1 << 13;
constexpr uint32_t MAX_POINT_LIGHTS = 1 << 13;
constexpr uint32_t MAX_BONES = 1 << 14;
constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 256;
constexpr uint32_t MAX_DECALS = 32;

constexpr uint32_t CLUSTER_X = 16, CLUSTER_Y = 9, CLUSTER_Z = 24;
constexpr uint32_t CLUSTER_SIZE = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;

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
    void UpdateGlobalIndexBuffer(vk::CommandBuffer& commandBuffer);

    void SpawnDecal(glm::vec3 normal, glm::vec3 position, glm::vec2 size, std::string albedoName);
    void ResetDecals();

    const vk::DescriptorSet& GetSceneDescriptorSet(uint32_t frameIndex) const { return _sceneFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetStaticInstancesDescriptorSet(uint32_t frameIndex) const { return _staticInstancesFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetSkinnedInstancesDescriptorSet(uint32_t frameIndex) const { return _skinnedInstancesFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetPointLightDescriptorSet(uint32_t frameIndex) const { return _pointLightFrameData.at(frameIndex).descriptorSet; }
    const vk::DescriptorSet& GetClusterDescriptorSet() const { return _clusterData.descriptorSet; }
    const vk::DescriptorSet& GetClusterCullingDescriptorSet(uint32_t frameIndex) const { return _clusterCullingData.descriptorSets.at(frameIndex); }
    const vk::DescriptorSet& GetDecalDescriptorSet() const { return _decalDescriptorSet; }
    const vk::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return _sceneDescriptorSetLayout; }
    const vk::DescriptorSetLayout& GetObjectInstancesDescriptorSetLayout() const { return _objectInstancesDSL; }
    const vk::DescriptorSetLayout& GetPointLightDescriptorSetLayout() const { return _pointLightDSL; }
    const vk::DescriptorSetLayout& GetHZBDescriptorSetLayout() const { return _hzbImageDSL; }
    const vk::DescriptorSetLayout& GetClusterDescriptorSetLayout() const { return _clusterDescriptorSetLayout; }
    const vk::DescriptorSetLayout& GetClusterCullingDescriptorSetLayout() const { return _clusterCullingDescriptorSetLayout; }
    const vk::DescriptorSetLayout& GetDecalDescriptorSetLayout() const { return _decalDescriptorSetLayout; }

    vk::DescriptorSetLayout DrawBufferLayout() const { return _drawBufferDSL; }
    ResourceHandle<Buffer> StaticDrawBuffer(uint32_t frameIndex) const { return _staticDraws[frameIndex].buffer; }
    vk::DescriptorSet StaticDrawDescriptorSet(uint32_t frameIndex) const { return _staticDraws[frameIndex].descriptorSet; }
    ResourceHandle<Buffer> SkinnedDrawBuffer(uint32_t frameIndex) const { return _skinnedDraws[frameIndex].buffer; }
    vk::DescriptorSet SkinnedDrawDescriptorSet(uint32_t frameIndex) const { return _skinnedDraws[frameIndex].descriptorSet; }

    const vk::DescriptorSetLayout GetSkinDescriptorSetLayout() const { return _skinDescriptorSetLayout; }
    const vk::DescriptorSet GetSkinDescriptorSet(uint32_t frameIndex) const { return _skinDescriptorSets[frameIndex]; }

    uint32_t StaticDrawCount() const { return _staticDrawCommands.size(); };
    const std::vector<DrawIndexedIndirectCommand>& StaticDrawCommands() const { return _staticDrawCommands; }
    ResourceHandle<Buffer>& GetClusterBuffer() { return _clusterData.buffer; }
    ResourceHandle<Buffer>& GetClusterCullingBuffer(uint32_t index) { return _clusterCullingData.buffers.at(index); }
    ResourceHandle<Buffer>& GetGlobalIndexBuffer() { return _clusterCullingData.globalIndexBuffer; }

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

    ResourceHandle<GPUImage> StaticShadow() const { return _staticShadowImage; }
    ResourceHandle<GPUImage> DynamicShadow() const { return _dynamicShadowImage; }
    bool ShouldUpdateShadows() const { return _shouldUpdateShadows; }

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
        float poissonWorldOffset;
        float poissonConstant;
    };

    struct alignas(16) PointLightData
    {
        glm::vec3 position;
        float range;
        glm::vec3 color;
        float intensity;
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
        uint32_t staticShadowMapIndex;
        uint32_t dynamicShadowMapIndex;

        glm::vec3 fogColor;
        float fogDensity;
        float fogHeight;
    };

    struct alignas(32) InstanceData
    {
        glm::mat4 model;

        uint32_t materialIndex;
        float boundingRadius;
        uint32_t boneOffset;
        bool isStaticDraw;
    };

    struct alignas(16) DecalData
    {
        glm::mat4 invModel;
        glm::vec3 orientation;
        uint32_t albedoIndex;
    };

    struct alignas(16) DecalArray
    {
        std::array<DecalData, MAX_DECALS> decals;
        uint32_t count = 0;
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

    struct ClusterData
    {
        ResourceHandle<Buffer> buffer;
        vk::DescriptorSet descriptorSet;
    };

    struct ClusterCullingData
    {
        std::array<ResourceHandle<Buffer>, 2> buffers;
        ResourceHandle<Buffer> globalIndexBuffer;
        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
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

    vk::DescriptorSetLayout _clusterDescriptorSetLayout;
    ClusterData _clusterData;

    vk::DescriptorSetLayout _clusterCullingDescriptorSetLayout;
    ClusterCullingData _clusterCullingData;

    DecalArray _decals;
    ResourceHandle<GPUImage>& GetDecalImage(std::string fileName);
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _decalImages;

    ResourceHandle<Buffer> _decalBuffer;
    vk::DescriptorSet _decalDescriptorSet;
    vk::DescriptorSetLayout _decalDescriptorSetLayout;

    std::vector<DrawIndexedIndirectCommand> _staticDrawCommands;
    std::vector<DrawIndexedIndirectCommand> _skinnedDrawCommands;
    bool _shouldUpdateShadows = false;

    CameraResource _mainCamera;
    CameraResource _directionalLightShadowCamera;

    std::unique_ptr<CameraBatch> _mainCameraBatch;
    std::unique_ptr<CameraBatch> _shadowCameraBatch;

    ResourceHandle<GPUImage> _staticShadowImage;
    ResourceHandle<GPUImage> _dynamicShadowImage;
    ResourceHandle<Sampler> _shadowSampler;

    vk::DescriptorSetLayout _skinDescriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _skinDescriptorSets;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _skinTransformBuffers;

    vk::DescriptorSetLayout _visibilityDSL;
    vk::DescriptorSetLayout _redirectDSL;
    vk::DescriptorSetLayout _hzbImageDSL;

    std::unordered_map<entt::entity, uint32_t> _skeletonBoneOffset {};

    void UpdateSceneData(uint32_t frameIndex);
    void UpdatePointLightArray(uint32_t frameIndex);
    void UpdateObjectInstancesData(uint32_t frameIndex);
    void UpdateDirectionalLightData(SceneData& scene, uint32_t frameIndex);
    void UpdatePointLightData(PointLightArray& pointLightArray, uint32_t frameIndex);
    void UpdateCameraData(uint32_t frameIndex);
    void UpdateSkinBuffers(uint32_t frameIndex);
    void UpdateDecalBuffer();

    void InitializeSceneBuffers();
    void InitializePointLightBuffer();
    void InitializeClusterBuffer();
    void InitializeClusterCullingBuffers();
    void InitializeObjectInstancesBuffers();
    void InitializeSkinBuffers();
    void InitializeDecalBuffer();

    void CreateSceneDescriptorSetLayout();
    void CreatePointLightDescriptorSetLayout();
    void CreateClusterDescriptorSetLayout();
    void CreateClusterCullingDescriptorSetLayout();
    void CreateObjectInstanceDescriptorSetLayout();
    void CreateSkinDescriptorSetLayout();
    void CreateHZBDescriptorSetLayout();
    void CreateDecalDescriptorSetLayout();

    void CreateSceneDescriptorSets();
    void CreatePointLightDescriptorSets();
    void CreateClusterDescriptorSet();
    void CreateClusterCullingDescriptorSet();
    void CreateObjectInstancesDescriptorSets();
    void CreateSkinDescriptorSets();
    void CreateDecalDescriptorSet();

    void UpdateSceneDescriptorSet(uint32_t frameIndex);
    void UpdatePointLightDescriptorSet(uint32_t frameIndex);
    void UpdateAtomicGlobalDescriptorSet(uint32_t frameIndex);
    void UpdateObjectInstancesDescriptorSet(uint32_t frameIndex);
    void UpdateSkinDescriptorSet(uint32_t frameIndex);

    void CreateSceneBuffers();
    void CreatePointLightBuffer();
    void CreateClusterBuffer();
    void CreateClusterCullingBuffers();
    void CreateObjectInstancesBuffers();
    void CreateSkinBuffers();
    void CreateDecalBuffer();

    void InitializeIndirectDrawBuffer();
    void InitializeIndirectDrawDescriptor();

    void WriteDraws(uint32_t frameIndex);

    void CreateShadowMapResources();
};