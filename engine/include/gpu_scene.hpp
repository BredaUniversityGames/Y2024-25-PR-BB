#pragma once

struct SceneDescription;

struct GPUSceneCreation
{
    const VulkanBrain& brain;
    const SceneDescription& scene;
    ResourceHandle<Image> irradianceMap;
    ResourceHandle<Image> prefilterMap;
    ResourceHandle<Image> brdfLUTMap;
    ResourceHandle<Image> directionalShadowMap;
};

class GPUScene
{
public:
    GPUScene(const GPUSceneCreation& creation);
    ~GPUScene();

    void Update(uint32_t frameIndex);

    const SceneDescription& scene;
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
        vk::Buffer buffer;
        VmaAllocation bufferAllocation;
        void* bufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    const VulkanBrain& _brain;

    vk::DescriptorSetLayout _sceneDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _sceneFrameData;
    vk::DescriptorSetLayout _objectInstanceDescriptorSetLayout;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _objectInstanceFrameData;

    void UpdateSceneData(uint32_t frameIndex);
    void UpdateObjectInstanceData(uint32_t frameIndex);

    void InitializeSceneBuffers();
    void InitializeObjectInstanceBuffers();

    void CreateSceneDescriptorSetLayout();
    void CreateObjectInstanceDescriptorSetLayout();

    void CreateSceneDescriptorSets();
    void CreateObjectInstanceDescriptorSets();

    void UpdateSceneDescriptorSet(uint32_t frameIndex);
    void UpdateObjectInstanceDescriptorSet(uint32_t frameIndex);

    void CreateSceneBuffers();
    void CreateObjectInstanceBuffers();
};