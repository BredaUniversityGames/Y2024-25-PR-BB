#include "gpu_scene.hpp"

#include "batch_buffer.hpp"
#include "components/camera_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <glm/gtc/matrix_transform.hpp>

GPUScene::GPUScene(const GPUSceneCreation& creation)
    : irradianceMap(creation.irradianceMap)
    , prefilterMap(creation.prefilterMap)
    , brdfLUTMap(creation.brdfLUTMap)
    , directionalShadowMap(creation.directionalShadowMap)
    , _context(creation.context)
    , _ecs(creation.ecs)
    , _mainCamera(creation.context)
    , _directionalLightShadowCamera(creation.context)
{
    InitializeSceneBuffers();
    InitializeObjectInstancesBuffers();

    InitializeIndirectDrawBuffer();
    InitializeIndirectDrawDescriptor();
}

GPUScene::~GPUScene()
{
    auto vkContext { _context->VulkanContext() };

    vkContext->Device().destroy(_drawBufferDescriptorSetLayout);
    vkContext->Device().destroy(_sceneDescriptorSetLayout);
    vkContext->Device().destroy(_objectInstancesDescriptorSetLayout);
}

void GPUScene::Update(uint32_t frameIndex)
{
    UpdateSceneData(frameIndex);
    UpdateCameraData(frameIndex);
    UpdateObjectInstancesData(frameIndex);
    WriteDraws(frameIndex);
}

void GPUScene::UpdateSceneData(uint32_t frameIndex)
{
    SceneData sceneData {};

    UpdateDirectionalLightData(sceneData, frameIndex);

    sceneData.irradianceIndex = irradianceMap.Index();
    sceneData.prefilterIndex = prefilterMap.Index();
    sceneData.brdfLUTIndex = brdfLUTMap.Index();
    sceneData.shadowMapIndex = directionalShadowMap.Index();

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_sceneFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, &sceneData, sizeof(SceneData));
}

void GPUScene::UpdateObjectInstancesData(uint32_t frameIndex)
{
    static std::vector<InstanceData> instances { MAX_INSTANCES };
    uint32_t count = 0;

    _drawCommands.clear();

    _staticDrawRange.start = 0;

    auto staticMeshView = _ecs->registry.view<StaticMeshComponent, WorldMatrixComponent>();

    for (auto entity : staticMeshView)
    {
        auto meshComponent = staticMeshView.get<StaticMeshComponent>(entity);
        auto transformComponent = staticMeshView.get<WorldMatrixComponent>(entity);

        auto resources { _context->Resources() };

        auto mesh = resources->MeshResourceManager().Access(meshComponent.mesh);
        assert(count < instances.size() && "Reached the limit of instance data available for the meshes");
        assert(resources->MaterialResourceManager().IsValid(mesh->material) && "There should always be a material available");

        instances[count].model = TransformHelpers::GetWorldMatrix(transformComponent);
        instances[count].materialIndex = mesh->material.Index();
        instances[count].boundingRadius = mesh->boundingRadius;

        _drawCommands.emplace_back(DrawIndexedIndirectCommand {
            .command = {
                .indexCount = mesh->count,
                .instanceCount = 1,
                .firstIndex = mesh->indexOffset,
                .vertexOffset = static_cast<int32_t>(mesh->vertexOffset),
                .firstInstance = 0,
            },
        });

        count++;
    }

    _staticDrawRange.count = count;
    _skinnedDrawRange.start = count;

    auto skinnedMeshView = _ecs->registry.view<SkinnedMeshComponent, WorldMatrixComponent>();

    for (auto entity : skinnedMeshView)
    {
        auto meshComponent = skinnedMeshView.get<SkinnedMeshComponent>(entity);
        auto transformComponent = skinnedMeshView.get<WorldMatrixComponent>(entity);

        auto resources { _context->Resources() };

        auto mesh = resources->MeshResourceManager().Access(meshComponent.mesh);
        assert(count < instances.size() && "Reached the limit of instance data available for the meshes");
        assert(resources->MaterialResourceManager().IsValid(mesh->material) && "There should always be a material available");

        instances[count].model = TransformHelpers::GetWorldMatrix(transformComponent);
        instances[count].materialIndex = mesh->material.Index();
        instances[count].boundingRadius = mesh->boundingRadius;

        _drawCommands.emplace_back(DrawIndexedIndirectCommand {
            .command = {
                .indexCount = mesh->count,
                .instanceCount = 1,
                .firstIndex = mesh->indexOffset,
                .vertexOffset = static_cast<int32_t>(mesh->vertexOffset),
                .firstInstance = 0,
            },
        });

        count++;
    }

    _skinnedDrawRange.count = count - _staticDrawRange.count;

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_objectInstancesFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, instances.data(), instances.size() * sizeof(InstanceData));
}

void GPUScene::UpdateDirectionalLightData(SceneData& scene, uint32_t frameIndex)
{
    auto directionalLightView = _ecs->registry.view<DirectionalLightComponent, TransformComponent>();
    bool directionalLightIsSet = false;

    for (const auto& [entity, directionalLightComponent, transformComponent] : directionalLightView.each())
    {
        if (directionalLightIsSet)
        {
            bblog::warn("Only 1 directional light is supported, the first one available will be used.");
            return;
        }

        glm::vec3 eulerRotation = glm::eulerAngles(TransformHelpers::GetLocalRotation(transformComponent));
        glm::vec3 direction = glm::vec3(
            cos(eulerRotation.y) * cos(eulerRotation.x),
            sin(eulerRotation.x),
            sin(eulerRotation.y) * cos(eulerRotation.x));

        float orthographicsSize = directionalLightComponent.orthographicSize;
        glm::mat4 depthProjectionMatrix = glm::ortho(-orthographicsSize, orthographicsSize, -orthographicsSize, orthographicsSize, directionalLightComponent.nearPlane, directionalLightComponent.farPlane);
        depthProjectionMatrix[1][1] *= -1;

        glm::vec3 position = TransformHelpers::GetLocalPosition(transformComponent);
        const glm::mat4 lightView = glm::lookAt(position, position - direction, glm::vec3(0, 1, 0));

        DirectionalLightData& directionalLightData = scene.directionalLight;
        directionalLightData.lightVP = depthProjectionMatrix * lightView;
        directionalLightData.depthBiasMVP = DirectionalLightComponent::BIAS_MATRIX * directionalLightData.lightVP;
        directionalLightData.direction = glm::vec4(direction, directionalLightComponent.shadowBias);
        directionalLightData.color = glm::vec4(directionalLightComponent.color, 1.0f);

        CameraComponent camera {
            .projection = CameraComponent::Projection::eOrthographic,
            .nearPlane = directionalLightComponent.nearPlane,
            .farPlane = directionalLightComponent.farPlane,
            .orthographicSize = directionalLightComponent.orthographicSize,
            .aspectRatio = directionalLightComponent.aspectRatio,
        };

        _directionalLightShadowCamera.Update(frameIndex, transformComponent, camera);

        directionalLightIsSet = true;
    }
}

void GPUScene::UpdateCameraData(uint32_t frameIndex)
{
    auto cameraView = _ecs->registry.view<CameraComponent, TransformComponent>();
    bool mainCameraIsSet = false;

    for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
    {
        if (mainCameraIsSet)
        {
            bblog::warn("Only 1 camera is supported, the first one available will be used.");
            return;
        }

        _mainCamera.Update(frameIndex, transformComponent, cameraComponent);

        mainCameraIsSet = true;
    }
}

void GPUScene::InitializeSceneBuffers()
{
    CreateSceneBuffers();
    CreateSceneDescriptorSetLayout();
    CreateSceneDescriptorSets();
}

void GPUScene::InitializeObjectInstancesBuffers()
{
    CreateObjectInstancesBuffers();
    CreateObjectInstanceDescriptorSetLayout();
    CreateObjectInstancesDescriptorSets();
}

void GPUScene::CreateSceneDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings(1);

    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<std::string_view> names { "SceneUBO" };

    _sceneDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
}

void GPUScene::CreateObjectInstanceDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute,
        .pImmutableSamplers = nullptr,
    };

    std::vector<vk::DescriptorSetLayoutBinding> bindings { descriptorSetLayoutBinding };

    std::vector<std::string_view> names { "InstanceData" };

    _objectInstancesDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
}

void GPUScene::CreateSceneDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _sceneDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _context->VulkanContext()->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating object instance descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _sceneFrameData[i].descriptorSet = descriptorSets[i];
        UpdateSceneDescriptorSet(i);
    }
}

void GPUScene::CreateObjectInstancesDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _objectInstancesDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _context->VulkanContext()->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating object instance descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _objectInstancesFrameData[i].descriptorSet = descriptorSets[i];
        UpdateObjectInstancesDescriptorSet(i);
    }
}

void GPUScene::UpdateSceneDescriptorSet(uint32_t frameIndex)
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_sceneFrameData[frameIndex].buffer);

    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer->buffer;
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

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GPUScene::UpdateObjectInstancesDescriptorSet(uint32_t frameIndex)
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_objectInstancesFrameData[frameIndex].buffer);

    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _objectInstancesFrameData[frameIndex].descriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GPUScene::CreateSceneBuffers()
{
    for (size_t i = 0; i < _sceneFrameData.size(); ++i)
    {
        std::string name = "[] Scene UBO";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(SceneData))
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer)
            .SetName(name);

        _sceneFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void GPUScene::CreateObjectInstancesBuffers()
{
    vk::DeviceSize bufferSize = sizeof(InstanceData) * MAX_INSTANCES;

    for (size_t i = 0; i < _objectInstancesFrameData.size(); ++i)
    {
        std::string name = "[] Object instances data";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(bufferSize)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
            .SetName(name);

        _objectInstancesFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void GPUScene::InitializeIndirectDrawBuffer()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        BufferCreation creation {};
        creation.SetSize(sizeof(DrawIndexedIndirectCommand) * MAX_MESHES + sizeof(uint32_t))
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetIsMappable(true)
            .SetName("Indirect draw buffer");

        _indirectDrawFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void GPUScene::InitializeIndirectDrawDescriptor()
{
    auto vkContext { _context->VulkanContext() };

    vk::DescriptorSetLayoutBinding layoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
    };

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &layoutBinding,
    };

    util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_drawBufferDescriptorSetLayout), "Failed creating descriptor set layout!");

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _drawBufferDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = vkContext->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _indirectDrawFrameData[i].descriptorSet = descriptorSets[i];

        const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_indirectDrawFrameData[i].buffer);

        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = buffer->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = vk::WholeSize;

        vk::WriteDescriptorSet bufferWrite {};
        bufferWrite.dstSet = _indirectDrawFrameData[i].descriptorSet;
        bufferWrite.dstBinding = 0;
        bufferWrite.dstArrayElement = 0;
        bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        bufferWrite.descriptorCount = 1;
        bufferWrite.pBufferInfo = &bufferInfo;

        vkContext->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
    }
}

void GPUScene::WriteDraws(uint32_t frameIndex)
{
    assert(_drawCommands.size() < MAX_INSTANCES && "Too many draw commands");

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_indirectDrawFrameData[frameIndex].buffer);

    std::memcpy(buffer->mappedPtr, _drawCommands.data(), _drawCommands.size() * sizeof(DrawIndexedIndirectCommand));
}
