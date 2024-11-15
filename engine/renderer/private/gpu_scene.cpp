#include "gpu_scene.hpp"

#include "batch_buffer.hpp"
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
{
    InitializeSceneBuffers();
    InitializeObjectInstancesBuffers();

    InitializeIndirectDrawBuffer();
    InitializeIndirectDrawDescriptor();
}

GPUScene::~GPUScene()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        resources->BufferResourceManager().Destroy(_sceneFrameData[i].buffer);
        resources->BufferResourceManager().Destroy(_objectInstancesFrameData[i].buffer);
        resources->BufferResourceManager().Destroy(_indirectDrawFrameData[i].buffer);
    }

    vkContext->Device().destroy(_drawBufferDescriptorSetLayout);
    vkContext->Device().destroy(_sceneDescriptorSetLayout);
    vkContext->Device().destroy(_objectInstancesDescriptorSetLayout);
}

void GPUScene::Update(const SceneDescription& scene, uint32_t frameIndex)
{
    UpdateSceneData(scene, frameIndex);
    UpdateObjectInstancesData(frameIndex);
    WriteDraws(frameIndex);
}

void GPUScene::UpdateSceneData(const SceneDescription& scene, uint32_t frameIndex)
{
    SceneData sceneData {};

    const DirectionalLight& light = scene.directionalLight;

    // Calculate light direction from Euler rotation
    glm::vec3 direction = glm::vec3(
        cos(light.camera.eulerRotation.y) * cos(light.camera.eulerRotation.x),
        sin(light.camera.eulerRotation.x),
        sin(light.camera.eulerRotation.y) * cos(light.camera.eulerRotation.x));

    const glm::mat4 lightView = glm::lookAt(light.camera.position, light.camera.position - direction, glm::vec3(0, 1, 0));
    glm::mat4 depthProjectionMatrix = glm::ortho(-light.camera.orthographicSize, light.camera.orthographicSize, -light.camera.orthographicSize, light.camera.orthographicSize, light.camera.nearPlane, light.camera.farPlane);
    depthProjectionMatrix[1][1] *= -1;

    DirectionalLightData& directionalLightData = sceneData.directionalLight;
    directionalLightData.lightVP = depthProjectionMatrix * lightView;
    directionalLightData.depthBiasMVP = DirectionalLight::BIAS_MATRIX * directionalLightData.lightVP;
    directionalLightData.direction = glm::vec4(direction, light.shadowBias);

    sceneData.irradianceIndex = irradianceMap.Index();
    sceneData.prefilterIndex = prefilterMap.Index();
    sceneData.brdfLUTIndex = brdfLUTMap.Index();
    sceneData.shadowMapIndex = directionalShadowMap.Index();

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_sceneFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, &sceneData, sizeof(SceneData));
}

void GPUScene::UpdateObjectInstancesData(uint32_t frameIndex)
{
    std::array<InstanceData, MAX_MESHES> instances {};
    uint32_t count = 0;

    _drawCommands.clear();

    auto meshView = _ecs->registry.view<StaticMeshComponent, WorldMatrixComponent>();

    meshView.each([this, &instances, &count](const auto meshComponent, const auto transformComponent)
        {
            auto resources{ _context->Resources() };

            auto mesh = resources->MeshResourceManager().Access(meshComponent.mesh);
            for (const auto& primitive : mesh->primitives)
            {
                assert(count < MAX_MESHES && "Reached the limit of instance data available for the meshes");
                assert(resources->MaterialResourceManager().IsValid(primitive.material) && "There should always be a material available");

                instances[count].model = TransformHelpers::GetWorldMatrix(transformComponent);
                instances[count].materialIndex = primitive.material.Index();
                instances[count].boundingRadius = primitive.boundingRadius;

                _drawCommands.emplace_back(vk::DrawIndexedIndirectCommand {
                    .indexCount = primitive.count,
                    .instanceCount = 1,
                    .firstIndex = primitive.indexOffset,
                    .vertexOffset = static_cast<int32_t>(primitive.vertexOffset),
                    .firstInstance = 0 });

                count++;
            } });

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_objectInstancesFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, instances.data(), instances.size() * sizeof(InstanceData));
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
        creation.SetSize(sizeof(vk::DrawIndexedIndirectCommand) * MAX_MESHES + sizeof(uint32_t))
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

    std::memcpy(buffer->mappedPtr, _drawCommands.data(), _drawCommands.size() * sizeof(vk::DrawIndexedIndirectCommand));

    // Write draw count in the final 4 bytes of the indirect draw buffer.
    uint32_t drawCount = _drawCommands.size();
    std::byte* ptr = static_cast<std::byte*>(buffer->mappedPtr);
    std::memcpy(ptr + IndirectCountOffset(), &drawCount, sizeof(uint32_t));
}
