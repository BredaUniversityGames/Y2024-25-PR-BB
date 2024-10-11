#pragma once

struct SceneDescription;
class GPUScene;

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
};

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
};