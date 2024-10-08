#include "gpu_scene.hpp"
#include "batch_buffer.hpp"
#include "vulkan_helper.hpp"

GPUScene::GPUScene(const GPUSceneCreation& creation) :
    _brain(creation.brain)
    , scene(creation.scene)
    , irradianceMap(creation.irradianceMap)
    , prefilterMap(creation.prefilterMap)
    , brdfLUTMap(creation.brdfLUTMap)
    , directionalShadowMap(creation.directionalShadowMap)
{
    InitializeSceneBuffers();
    InitializeObjectInstanceBuffers();
}

GPUScene::~GPUScene()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _sceneFrameData[i].bufferAllocation);
        vmaDestroyBuffer(_brain.vmaAllocator, _sceneFrameData[i].buffer, _sceneFrameData[i].bufferAllocation);

        vmaUnmapMemory(_brain.vmaAllocator, _objectInstanceFrameData[i].bufferAllocation);
        vmaDestroyBuffer(_brain.vmaAllocator, _objectInstanceFrameData[i].buffer, _objectInstanceFrameData[i].bufferAllocation);
    }

    _brain.device.destroy(_sceneDescriptorSetLayout);
    _brain.device.destroy(_objectInstanceDescriptorSetLayout);
}

void GPUScene::Update(uint32_t frameIndex)
{
    UpdateSceneData(frameIndex);
    UpdateObjectInstanceData(frameIndex);
}

void GPUScene::UpdateSceneData(uint32_t frameIndex)
{
    SceneData sceneData{};

    const DirectionalLight& light = scene.directionalLight;

    const glm::mat4 lightView = glm::lookAt(light.targetPos - normalize(light.lightDir) * light.sceneDistance, light.targetPos, glm::vec3(0, 1, 0));
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-light.orthoSize, light.orthoSize, -light.orthoSize, light.orthoSize, light.nearPlane, light.farPlane);
    depthProjectionMatrix[1][1] *= -1;

    DirectionalLightData& directionalLightData = sceneData.directionalLight;
    directionalLightData.lightVP = depthProjectionMatrix * lightView;
    directionalLightData.depthBiasMVP = light.biasMatrix * directionalLightData.lightVP;
    directionalLightData.direction = glm::vec4(light.targetPos - normalize(light.lightDir) * light.sceneDistance, light.shadowBias);

    sceneData.irradianceIndex = irradianceMap.index;
    sceneData.prefilterIndex = prefilterMap.index;
    sceneData.brdfLUTIndex = brdfLUTMap.index;
    sceneData.shadowMapIndex = directionalShadowMap.index;

    memcpy(_sceneFrameData[frameIndex].bufferMapped, &sceneData, sizeof(SceneData));
}

void GPUScene::UpdateObjectInstanceData(uint32_t frameIndex)
{
    std::array<InstanceData, MAX_MESHES> instances{};
    uint32_t count = 0;

    for (auto& gameObject : scene.gameObjects)
    {
        for (auto& node : gameObject.model->hierarchy.allNodes)
        {
            for (const auto& primitive : node.mesh->primitives)
            {
                assert(count < MAX_MESHES && "Reached the limit of instance data available for the meshes");
                assert(_brain.GetMaterialResourceManager().IsValid(primitive.material) && "There should always be a material available");

                instances[count].model = gameObject.transform * node.transform;
                instances[count].materialIndex = primitive.material.index;

                count++;
            }
        }
    }

    memcpy(_objectInstanceFrameData[frameIndex].bufferMapped, instances.data(), instances.size() * sizeof(InstanceData));
}

void GPUScene::InitializeSceneBuffers()
{
    CreateSceneDescriptorSetLayout();
    CreateSceneDescriptorSets();
}

void GPUScene::InitializeObjectInstanceBuffers()
{
    CreateObjectInstanceDescriptorSetLayout();
    CreateObjectInstanceDescriptorSets();
}

void GPUScene::CreateSceneDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_sceneDescriptorSetLayout),
        "Failed creating scene descriptor set layout!");
}

void GPUScene::CreateObjectInstanceDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_objectInstanceDescriptorSetLayout),
        "Failed creating object instance descriptor set layout!");
}

void GPUScene::CreateSceneDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _sceneDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating object instance descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _sceneFrameData[i].descriptorSet = descriptorSets[i];
        UpdateSceneDescriptorSet(i);
    }
}

void GPUScene::CreateObjectInstanceDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _objectInstanceDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating object instance descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _objectInstanceFrameData[i].descriptorSet = descriptorSets[i];
        UpdateObjectInstanceDescriptorSet(i);
    }
}

void GPUScene::UpdateSceneDescriptorSet(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = _sceneFrameData[frameIndex].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _sceneFrameData[frameIndex].descriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GPUScene::UpdateObjectInstanceDescriptorSet(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = _objectInstanceFrameData[frameIndex].buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _objectInstanceFrameData[frameIndex].descriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GPUScene::CreateSceneBuffers()
{
    for (size_t i = 0; i < _sceneFrameData.size(); ++i)
    {
        util::CreateBuffer(_brain, sizeof(SceneData),
            vk::BufferUsageFlagBits::eUniformBuffer,
            _sceneFrameData[i].buffer, true, _sceneFrameData[i].bufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _sceneFrameData[i].bufferAllocation, &_sceneFrameData[i].bufferMapped),
            "Failed mapping memory for UBO!");
    }
}

void GPUScene::CreateObjectInstanceBuffers()
{
    vk::DeviceSize bufferSize = sizeof(InstanceData) * MAX_MESHES;

    for (size_t i = 0; i < _objectInstanceFrameData.size(); ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer,
            _objectInstanceFrameData[i].buffer, true, _objectInstanceFrameData[i].bufferAllocation,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _objectInstanceFrameData[i].bufferAllocation, &_objectInstanceFrameData[i].bufferMapped),
            "Failed mapping memory for UBO!");
    }
}