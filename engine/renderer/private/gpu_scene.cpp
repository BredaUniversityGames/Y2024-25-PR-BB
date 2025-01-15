#include "gpu_scene.hpp"

#include "batch_buffer.hpp"
#include "camera_batch.hpp"
#include "components/camera_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/joint_component.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/relationship_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_mesh_component.hpp"
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <tracy/Tracy.hpp>
#include <unordered_map>

GPUScene::GPUScene(const GPUSceneCreation& creation)
    : irradianceMap(creation.irradianceMap)
    , prefilterMap(creation.prefilterMap)
    , brdfLUTMap(creation.brdfLUTMap)
    , _context(creation.context)
    , _ecs(creation.ecs)
    , _mainCamera(creation.context, true)
    , _directionalLightShadowCamera(creation.context, false)
{
    InitializeSceneBuffers();
    InitializePointLightBuffer();
    InitializeObjectInstancesBuffers();
    InitializeSkinBuffers();

    CreateHZBDescriptorSetLayout();

    InitializeIndirectDrawBuffer();
    InitializeIndirectDrawDescriptor();

    CreateShadowMapResources();

    std::vector<vk::DescriptorSetLayoutBinding> bindingsVisibility {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics }
    };
    std::vector<std::string_view> namesVisibility { "VisibilityBuffer" };

    _visibilityDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindingsVisibility, namesVisibility);

    std::vector<vk::DescriptorSetLayoutBinding> bindingsRedirect {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics }
    };
    std::vector<std::string_view> namesRedirect { "RedirectBuffer" };

    _redirectDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindingsRedirect, namesRedirect);

    _mainCameraBatch = std::make_unique<CameraBatch>(_context, "Main Camera Batch", _mainCamera, creation.depthImage, _drawBufferDSL, _visibilityDSL, _redirectDSL);
    _shadowCameraBatch = std::make_unique<CameraBatch>(_context, "Shadow Camera Batch", _directionalLightShadowCamera, _shadowImage, _drawBufferDSL, _visibilityDSL, _redirectDSL);
}

GPUScene::~GPUScene()
{
    auto vkContext { _context->VulkanContext() };

    vkContext->Device().destroy(_drawBufferDSL);
    vkContext->Device().destroy(_sceneDescriptorSetLayout);
    vkContext->Device().destroy(_objectInstancesDSL);
    vkContext->Device().destroy(_skinDescriptorSetLayout);
    vkContext->Device().destroy(_pointLightDSL);
    vkContext->Device().destroy(_visibilityDSL);
    vkContext->Device().destroy(_redirectDSL);
    vkContext->Device().destroy(_hzbImageDSL);
}

void GPUScene::Update(uint32_t frameIndex)
{
    ZoneScoped;
    UpdateSceneData(frameIndex);
    UpdatePointLightArray(frameIndex);
    UpdateCameraData(frameIndex);
    UpdateObjectInstancesData(frameIndex);
    UpdateSkinBuffers(frameIndex);
    WriteDraws(frameIndex);
}

void GPUScene::UpdateSceneData(uint32_t frameIndex)
{
    SceneData sceneData {};

    UpdateDirectionalLightData(sceneData, frameIndex);

    sceneData.irradianceIndex = irradianceMap.Index();
    sceneData.prefilterIndex = prefilterMap.Index();
    sceneData.brdfLUTIndex = brdfLUTMap.Index();
    sceneData.shadowMapIndex = _shadowImage.Index();

    sceneData.fogColor = fogColor;
    sceneData.fogDensity = fogDensity;
    sceneData.fogHeight = fogHeight;

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_sceneFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, &sceneData, sizeof(SceneData));
}

void GPUScene::UpdatePointLightArray(uint32_t frameIndex)
{
    PointLightArray pointLightArray {};

    UpdatePointLightData(pointLightArray, frameIndex);

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_pointLightFrameData[frameIndex].buffer);
    memcpy(buffer->mappedPtr, &pointLightArray, sizeof(PointLightArray));
}

void GPUScene::UpdateObjectInstancesData(uint32_t frameIndex)
{
    static std::vector<InstanceData> staticInstances { MAX_STATIC_INSTANCES };
    uint32_t count = 0;

    _staticDrawCommands.clear();

    auto staticMeshView = _ecs.GetRegistry().view<StaticMeshComponent, WorldMatrixComponent>();

    for (auto entity : staticMeshView)
    {
        const auto& meshComponent = staticMeshView.get<StaticMeshComponent>(entity);
        const auto& transformComponent = staticMeshView.get<WorldMatrixComponent>(entity);

        auto resources { _context->Resources() };

        auto mesh = resources->MeshResourceManager().Access(meshComponent.mesh);
        assert(count < staticInstances.size() && "Reached the limit of instance data available for the meshes");
        assert(resources->MaterialResourceManager().IsValid(mesh->material) && "There should always be a material available");

        staticInstances[count].model = TransformHelpers::GetWorldMatrix(transformComponent);
        staticInstances[count].materialIndex = mesh->material.Index();
        staticInstances[count].boundingRadius = mesh->boundingRadius;

        _staticDrawCommands.emplace_back(DrawIndexedIndirectCommand {
            .command = {
                .indexCount = mesh->count,
                .instanceCount = 0,
                .firstIndex = mesh->indexOffset,
                .vertexOffset = static_cast<int32_t>(mesh->vertexOffset),
                .firstInstance = 0,
            },
        });

        count++;
    }

    static std::vector<InstanceData> skinnedInstances { MAX_SKINNED_INSTANCES };
    _skinnedDrawCommands.clear();
    count = 0;

    auto skinnedMeshView = _ecs.GetRegistry().view<SkinnedMeshComponent, WorldMatrixComponent>();

    for (auto entity : skinnedMeshView)
    {
        SkinnedMeshComponent skinnedMeshComponent = skinnedMeshView.get<SkinnedMeshComponent>(entity);
        auto transformComponent = skinnedMeshView.get<WorldMatrixComponent>(entity);

        auto resources { _context->Resources() };

        auto mesh = resources->MeshResourceManager().Access(skinnedMeshComponent.mesh);
        assert(count < skinnedInstances.size() && "Reached the limit of instance data available for the meshes");
        assert(resources->MaterialResourceManager().IsValid(mesh->material) && "There should always be a material available");

        skinnedInstances[count].model = TransformHelpers::GetWorldMatrix(transformComponent);
        skinnedInstances[count].materialIndex = mesh->material.Index();
        skinnedInstances[count].boundingRadius = mesh->boundingRadius;
        skinnedInstances[count].boneOffset = _ecs.GetRegistry().get<SkeletonComponent>(skinnedMeshComponent.skeletonEntity).boneOffset;

        _skinnedDrawCommands.emplace_back(DrawIndexedIndirectCommand {
            .command = {
                .indexCount = mesh->count,
                .instanceCount = 0,
                .firstIndex = mesh->indexOffset,
                .vertexOffset = static_cast<int32_t>(mesh->vertexOffset),
                .firstInstance = 0,
            },
        });

        count++;
    }

    const Buffer* staticInstancesBuffer = _context->Resources()->BufferResourceManager().Access(_staticInstancesFrameData[frameIndex].buffer);
    memcpy(staticInstancesBuffer->mappedPtr, staticInstances.data(), staticInstances.size() * sizeof(InstanceData));

    const Buffer* skinnedInstancesBuffer = _context->Resources()->BufferResourceManager().Access(_skinnedInstancesFrameData[frameIndex].buffer);
    memcpy(skinnedInstancesBuffer->mappedPtr, skinnedInstances.data(), skinnedInstances.size() * sizeof(InstanceData));
}

void GPUScene::UpdateDirectionalLightData(SceneData& scene, uint32_t frameIndex)
{
    auto directionalLightView = _ecs.GetRegistry().view<DirectionalLightComponent, TransformComponent>();
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
            .reversedZ = _directionalLightShadowCamera.UsesReverseZ(),
        };

        _directionalLightShadowCamera.Update(frameIndex, transformComponent, camera, lightView, depthProjectionMatrix);

        directionalLightIsSet = true;
    }
}

void GPUScene::UpdatePointLightData(PointLightArray& pointLightArray, MAYBE_UNUSED uint32_t frameIndex)
{
    auto pointLightView = _ecs.GetRegistry().view<PointLightComponent, TransformComponent>();
    uint32_t pointLightCount = 0;

    for (const auto& [entity, pointLightComponent, transformComponent] : pointLightView.each())
    {
        if (pointLightCount >= MAX_POINT_LIGHTS)
        {
            bblog::warn("Reached the limit of point lights available");
            break;
        }

        PointLightData& pointLightData = pointLightArray.lights[pointLightCount];
        pointLightData.position = glm::vec4(TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity)[3]);
        pointLightData.color
            = glm::vec4(pointLightComponent.color, 1.0f);
        pointLightData.range = pointLightComponent.range;
        pointLightData.attenuation = pointLightComponent.attenuation;

        pointLightCount++;
    }
    pointLightArray.count = pointLightCount;
}

void GPUScene::UpdateCameraData(uint32_t frameIndex)
{
    auto cameraView = _ecs.GetRegistry().view<CameraComponent, TransformComponent>();
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

void GPUScene::UpdateSkinBuffers(uint32_t frameIndex)
{
    auto jointView = _ecs.GetRegistry().view<JointComponent, WorldMatrixComponent>();
    static std::array<glm::mat4, MAX_BONES> skinMatrices {};
    static std::unordered_map<entt::entity, uint32_t> skeletonBoneOffset {};
    skeletonBoneOffset.clear();

    // Sort joints based on their skeletons. This means that all joints that share a skeleton will be kept together.
    _ecs.GetRegistry().sort<JointComponent>([](const JointComponent& a, const JointComponent& b)
        { return a.skeletonEntity < b.skeletonEntity; });

    // While traversing all joints we keep track of skeleton we're on, this helps for determining when we switch to another skeleton.
    entt::entity lastSkeleton = entt::null;
    // We track the offset that we need for each skeleton, so the bones are set properly in the buffer.
    uint32_t offset = 0;
    // The highest index is used to determine what the count of joints is, so we can use that for our offset.
    uint32_t highestIndex = 0;
    for (entt::entity entity : jointView)
    {
        const auto& joint = jointView.get<JointComponent>(entity);
        const auto& matrixComponent = jointView.get<WorldMatrixComponent>(entity);
        const glm::mat4& worldTransform = TransformHelpers::GetWorldMatrix(matrixComponent);

        if (lastSkeleton != joint.skeletonEntity)
        {
            lastSkeleton = joint.skeletonEntity;
            offset += highestIndex + 1;

            skeletonBoneOffset[lastSkeleton] = offset;
            highestIndex = 0;
        }

        highestIndex = glm::max(highestIndex, joint.jointIndex);

        skinMatrices[offset + joint.jointIndex] = worldTransform * joint.inverseBindMatrix;
    }

    // Apply all the offsets, to the skeletons, so we know their respective offsets for in the shader.
    auto skeletonView = _ecs.GetRegistry().view<SkeletonComponent>();
    for (auto [entity, offset] : skeletonBoneOffset)
    {
        auto& skeleton = skeletonView.get<SkeletonComponent>(entity);
        skeleton.boneOffset = offset;
    }

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_skinBuffers[frameIndex]);
    std::memcpy(buffer->mappedPtr, skinMatrices.data(), sizeof(glm::mat4) * skinMatrices.size());
}

void GPUScene::InitializeSceneBuffers()
{
    CreateSceneBuffers();
    CreateSceneDescriptorSetLayout();
    CreateSceneDescriptorSets();
}

void GPUScene::InitializePointLightBuffer()
{
    CreatePointLightBuffer();
    CreatePointLightDescriptorSetLayout();
    CreatePointLightDescriptorSets();
}

void GPUScene::InitializeObjectInstancesBuffers()
{
    CreateObjectInstancesBuffers();
    CreateObjectInstanceDescriptorSetLayout();
    CreateObjectInstancesDescriptorSets();
}

void GPUScene::InitializeSkinBuffers()
{
    CreateSkinBuffers();
    CreateSkinDescriptorSetLayout();
    CreateSkinDescriptorSets();
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

void GPUScene::CreatePointLightDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<std::string_view> names { "PointLightSSBO" };

    _pointLightDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
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

    _objectInstancesDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
}

void GPUScene::CreateSkinDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute,
    };

    std::vector<vk::DescriptorSetLayoutBinding> bindings { binding };
    std::vector<std::string_view> names { "SkinningMatrices" };
    _skinDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
}

void GPUScene::CreateHZBDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
    bindings[0] = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
    };
    bindings[1] = {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
    };
    std::vector<std::string_view> names { "inputTexture", "outputTexture" };
    vk::DescriptorSetLayoutCreateInfo dslCreateInfo = vk::DescriptorSetLayoutCreateInfo {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    _hzbImageDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names, dslCreateInfo);
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

void GPUScene::CreatePointLightDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _pointLightDSL; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _context->VulkanContext()->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating point light descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _pointLightFrameData[i].descriptorSet = descriptorSets[i];
        UpdatePointLightDescriptorSet(i);
    }
}

void GPUScene::CreateObjectInstancesDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT * 2> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _objectInstancesDSL; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _context->VulkanContext()->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT * 2;
    allocateInfo.pSetLayouts = layouts.data();

    std::vector<vk::DescriptorSet> descriptorSets = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _staticInstancesFrameData[i].descriptorSet = descriptorSets[i * 2];
        _skinnedInstancesFrameData[i].descriptorSet = descriptorSets[i * 2 + 1];
        UpdateObjectInstancesDescriptorSet(i);
    }
}

void GPUScene::CreateSkinDescriptorSets()
{
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = { _skinDescriptorSetLayout, _skinDescriptorSetLayout, _skinDescriptorSetLayout };
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts.data(),
    };
    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, _skinDescriptorSets.data()),
        "Failed allocating object instance descriptor sets!");
    for (size_t i = 0; i < _skinDescriptorSets.size(); ++i)
    {
        UpdateSkinDescriptorSet(i);
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

void GPUScene::UpdatePointLightDescriptorSet(uint32_t frameIndex)
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_pointLightFrameData[frameIndex].buffer);

    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _pointLightFrameData[frameIndex].descriptorSet;
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void GPUScene::UpdateObjectInstancesDescriptorSet(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo staticBufferInfo {};
    staticBufferInfo.buffer = _context->Resources()->BufferResourceManager().Access(_staticInstancesFrameData[frameIndex].buffer)->buffer;
    staticBufferInfo.offset = 0;
    staticBufferInfo.range = vk::WholeSize;

    vk::DescriptorBufferInfo skinnedBufferInfo {};
    skinnedBufferInfo.buffer = _context->Resources()->BufferResourceManager().Access(_skinnedInstancesFrameData[frameIndex].buffer)->buffer;
    skinnedBufferInfo.offset = 0;
    skinnedBufferInfo.range = vk::WholeSize;

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites {};

    vk::WriteDescriptorSet& staticBufferWrite { descriptorWrites[0] };
    staticBufferWrite.dstSet = _staticInstancesFrameData[frameIndex].descriptorSet;
    staticBufferWrite.dstBinding = 0;
    staticBufferWrite.dstArrayElement = 0;
    staticBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    staticBufferWrite.descriptorCount = 1;
    staticBufferWrite.pBufferInfo = &staticBufferInfo;

    vk::WriteDescriptorSet& skinnedBufferWrite { descriptorWrites[1] };
    skinnedBufferWrite.dstSet = _skinnedInstancesFrameData[frameIndex].descriptorSet;
    skinnedBufferWrite.dstBinding = 0;
    skinnedBufferWrite.dstArrayElement = 0;
    skinnedBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    skinnedBufferWrite.descriptorCount = 1;
    skinnedBufferWrite.pBufferInfo = &skinnedBufferInfo;

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites, 0);
}

void GPUScene::UpdateSkinDescriptorSet(uint32_t frameIndex)
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_skinBuffers[frameIndex]);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _skinDescriptorSets[frameIndex],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
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

void GPUScene::CreatePointLightBuffer()
{
    for (size_t i = 0; i < _pointLightFrameData.size(); ++i)
    {
        std::string name = "[] PointLight SSBO";
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(PointLightArray))
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
            .SetName(name);

        _pointLightFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void GPUScene::CreateObjectInstancesBuffers()
{
    for (size_t i = 0; i < _staticInstancesFrameData.size(); ++i)
    {
        std::string name = "[] Static instances data";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(InstanceData) * MAX_STATIC_INSTANCES)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
            .SetName(name);

        _staticInstancesFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
    for (size_t i = 0; i < _skinnedInstancesFrameData.size(); ++i)
    {
        std::string name = "[] Skinned instances data";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(InstanceData) * MAX_SKINNED_INSTANCES)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
            .SetName(name);

        _skinnedInstancesFrameData[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}
void GPUScene::CreateSkinBuffers()
{
    for (uint32_t i = 0; i < _skinBuffers.size(); ++i)
    {
        BufferCreation creation {
            .size = sizeof(glm::mat4) * MAX_BONES,
            .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
            .isMappable = true,
            .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .name = "Skin matrices buffer",
        };
        _skinBuffers[i] = _context->Resources()->BufferResourceManager().Create(creation);

        const Buffer* skinBuffer = _context->Resources()->BufferResourceManager().Access(_skinBuffers[i]);
        for (uint32_t j = 0; j < MAX_BONES; ++j)
        {
            glm::mat4 data { 1.0f };
            std::memcpy(static_cast<std::byte*>(skinBuffer->mappedPtr) + sizeof(glm::mat4) * j, &data, sizeof(glm::mat4));
        }
    }
}

void GPUScene::InitializeIndirectDrawBuffer()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        BufferCreation creation {};
        creation.SetSize(sizeof(DrawIndexedIndirectCommand) * MAX_STATIC_INSTANCES)
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetIsMappable(true)
            .SetName("Static indirect draw buffer");

        _staticDraws[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        BufferCreation creation {};
        creation.SetSize(sizeof(DrawIndexedIndirectCommand) * MAX_SKINNED_INSTANCES)
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetIsMappable(true)
            .SetName("Skinned indirect draw buffer");

        _skinnedDraws[i].buffer = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void GPUScene::InitializeIndirectDrawDescriptor()
{
    auto vkContext { _context->VulkanContext() };

    std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
    bindings[0] = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAll,
    };
    std::vector<std::string_view> names { "DrawCommands" };

    _drawBufferDSL = PipelineBuilder::CacheDescriptorSetLayout(*vkContext, bindings, names);

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _drawBufferDSL; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = vkContext->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::vector<vk::DescriptorSet> descriptorSets = vkContext->Device().allocateDescriptorSets(allocateInfo);

    std::array<vk::DescriptorBufferInfo, MAX_FRAMES_IN_FLIGHT> bufferInfos;
    std::array<vk::WriteDescriptorSet, MAX_FRAMES_IN_FLIGHT> bufferWrites;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _staticDraws[i].descriptorSet = descriptorSets[i];

        vk::DescriptorBufferInfo& staticBufferInfo { bufferInfos[i] };
        staticBufferInfo.buffer = _context->Resources()->BufferResourceManager().Access(_staticDraws[i].buffer)->buffer;
        staticBufferInfo.offset = 0;
        staticBufferInfo.range = vk::WholeSize;

        bufferWrites[i].dstSet = _staticDraws[i].descriptorSet;
        bufferWrites[i].dstBinding = 0;
        bufferWrites[i].dstArrayElement = 0;
        bufferWrites[i].descriptorType = vk::DescriptorType::eStorageBuffer;
        bufferWrites[i].descriptorCount = 1;
        bufferWrites[i].pBufferInfo = &staticBufferInfo;
    }

    vkContext->Device().updateDescriptorSets(bufferWrites, {});

    descriptorSets = vkContext->Device().allocateDescriptorSets(allocateInfo);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _skinnedDraws[i].descriptorSet = descriptorSets[i];

        vk::DescriptorBufferInfo& skinnedBufferInfo { bufferInfos[i] };
        skinnedBufferInfo.buffer = _context->Resources()->BufferResourceManager().Access(_skinnedDraws[i].buffer)->buffer;
        skinnedBufferInfo.offset = 0;
        skinnedBufferInfo.range = vk::WholeSize;

        bufferWrites[i].dstSet = _skinnedDraws[i].descriptorSet;
        bufferWrites[i].dstBinding = 0;
        bufferWrites[i].dstArrayElement = 0;
        bufferWrites[i].descriptorType = vk::DescriptorType::eStorageBuffer;
        bufferWrites[i].descriptorCount = 1;
        bufferWrites[i].pBufferInfo = &skinnedBufferInfo;
    }

    vkContext->Device().updateDescriptorSets(bufferWrites, {});
}

void GPUScene::WriteDraws(uint32_t frameIndex)
{
    assert(_staticDrawCommands.size() < MAX_STATIC_INSTANCES && "Too many draw commands");

    const Buffer* staticBuffer = _context->Resources()->BufferResourceManager().Access(_staticDraws[frameIndex].buffer);
    const Buffer* skinnedBuffer = _context->Resources()->BufferResourceManager().Access(_skinnedDraws[frameIndex].buffer);

    std::memcpy(staticBuffer->mappedPtr, _staticDrawCommands.data(), _staticDrawCommands.size() * sizeof(DrawIndexedIndirectCommand));
    std::memcpy(skinnedBuffer->mappedPtr, _skinnedDrawCommands.data(), _skinnedDrawCommands.size() * sizeof(DrawIndexedIndirectCommand));
}

void GPUScene::CreateShadowMapResources()
{
    SamplerCreation shadowSamplerInfo {
        .name = "Shadow sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        .compareEnable = true,
        .compareOp = vk::CompareOp::eLessOrEqual,
    };
    shadowSamplerInfo.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToBorder);
    _shadowSampler = _context->Resources()->SamplerResourceManager().Create(shadowSamplerInfo);

    CPUImage shadowCreation {};
    shadowCreation
        .SetFormat(vk::Format::eD32Sfloat)
        .SetType(ImageType::eShadowMap)
        .SetSize(2048, 2048)
        .SetName("Shadow image")
        .SetFlags(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
    _shadowImage = _context->Resources()->ImageResourceManager().Create(shadowCreation, _shadowSampler);
}
