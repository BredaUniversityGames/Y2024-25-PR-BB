#include "camera.hpp"

#include "components/camera_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <ecs_module.hpp>
#include <glm/gtc/quaternion.hpp>

vk::DescriptorSetLayout CameraResource::_descriptorSetLayout;

CameraResource::CameraResource(const std::shared_ptr<GraphicsContext>& context, bool useReverseZ)
    : _context(context)
    , _useReverseZ(useReverseZ)
{
    CreateDescriptorSetLayout(context);
    CreateBuffers();
    CreateDescriptorSets();
}

CameraResource::~CameraResource()
{
    if (_descriptorSetLayout)
    {
        _context->VulkanContext()->Device().destroy(_descriptorSetLayout);
        _descriptorSetLayout = nullptr;
    }
}

void CameraResource::CreateDescriptorSetLayout(const std::shared_ptr<GraphicsContext>& context)
{
    if (_descriptorSetLayout)
    {
        return;
    }

    vk::DescriptorSetLayoutBinding descriptorSetBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute,
    };

    std::vector<vk::DescriptorSetLayoutBinding> bindings { descriptorSetBinding };
    std::vector<std::string_view> names { "CameraUBO" };

    _descriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*context->VulkanContext(), bindings, names);
}

void CameraResource::CreateBuffers()
{
    vk::DeviceSize bufferSize = sizeof(GPUCamera);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        std::string name = "[] Camera Buffer";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(bufferSize)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetIsMappable(true)
            .SetName(name);

        _buffers.at(i) = _context->Resources()->BufferResourceManager().Create(creation);
    }
}

void CameraResource::CreateDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = vkContext->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, _descriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < _descriptorSets.size(); ++i)
    {
        const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_buffers[i]);

        vk::DescriptorBufferInfo bufferInfo {
            .buffer = buffer->buffer,
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet bufferWrite {
            .dstSet = _descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo,
        };
        vkContext->Device().updateDescriptorSets({ bufferWrite }, {});
    }
}

glm::vec4 normalizePlane(glm::vec4 p)
{
    return p / glm::length(glm::vec3(p));
}

void CameraResource::Update(uint32_t currentFrame, const CameraComponent& camera, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& position)
{
    GPUCamera cameraBuffer {};

    cameraBuffer.view = view;

    cameraBuffer.inverseView = glm::inverse(cameraBuffer.view);

    cameraBuffer.proj = proj;
    cameraBuffer.fov = camera.fov;

    switch (camera.projection)
    {
    case CameraComponent::Projection::ePerspective:
    {
        glm::mat4 projT = glm::transpose(cameraBuffer.proj);

        glm::vec4 frustumX = normalizePlane(projT[3] + projT[0]);
        glm::vec4 frustumY = normalizePlane(projT[3] + projT[1]);

        cameraBuffer.frustum[0] = frustumX.x;
        cameraBuffer.frustum[1] = frustumX.z;
        cameraBuffer.frustum[2] = frustumY.y;
        cameraBuffer.frustum[3] = frustumY.z;
    }
    break;
    case CameraComponent::Projection::eOrthographic:
    {
        cameraBuffer.frustum[0] = -camera.orthographicSize;
        cameraBuffer.frustum[1] = camera.orthographicSize;
        cameraBuffer.frustum[2] = -camera.orthographicSize;
        cameraBuffer.frustum[3] = camera.orthographicSize;
    }
    default:
        break;
    }

    cameraBuffer.projectionType = static_cast<int32_t>(camera.projection);
    cameraBuffer.VP = cameraBuffer.proj * cameraBuffer.view;
    cameraBuffer.inverseProj = glm::inverse(cameraBuffer.proj);
    cameraBuffer.inverseVP = glm::inverse(cameraBuffer.VP);
    cameraBuffer.cameraPosition = position;

    cameraBuffer.skydomeMVP = cameraBuffer.view;
    cameraBuffer.skydomeMVP[3][0] = 0.0f;
    cameraBuffer.skydomeMVP[3][1] = 0.0f;
    cameraBuffer.skydomeMVP[3][2] = 0.0f;
    cameraBuffer.skydomeMVP = cameraBuffer.proj * cameraBuffer.skydomeMVP;

    cameraBuffer.zNear = camera.nearPlane;
    cameraBuffer.zFar = camera.farPlane;

    cameraBuffer.distanceCullingEnabled = true;
    cameraBuffer.cullingEnabled = true;

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_buffers[currentFrame]);
    std::memcpy(buffer->mappedPtr, &cameraBuffer, sizeof(cameraBuffer));
}

glm::mat4 CameraResource::CalculateProjectionMatrix(const CameraComponent& camera)
{
    glm::mat4 proj;

    switch (camera.projection)
    {
    case CameraComponent::Projection::ePerspective:
    {
        if (camera.reversedZ)
        {
            // Swapped far and near plane, since reverse Z is used.
            proj = glm::perspective(camera.fov, camera.aspectRatio, camera.farPlane, camera.nearPlane);
        }
        else
        {
            proj = glm::perspective(camera.fov, camera.aspectRatio, camera.nearPlane, camera.farPlane);
        }
    }
    break;
    case CameraComponent::Projection::eOrthographic:
    {
        float left = -camera.orthographicSize;
        float right = camera.orthographicSize;
        float bottom = -camera.orthographicSize;
        float top = camera.orthographicSize;

        if (camera.reversedZ)
        {
            // Swapped far and near plane, since reverse Z is used.
            proj = glm::ortho<float>(left, right, bottom, top, camera.farPlane, camera.nearPlane);
        }
        else
        {
            proj = glm::ortho<float>(left, right, bottom, top, camera.nearPlane, camera.farPlane);
        }
    }
    default:
        break;
    }

    proj[1][1] *= -1;

    return proj;
}

glm::mat4 CameraResource::CalculateViewMatrix(const glm::quat& rotation, const glm::vec3& position)
{
    glm::mat4 cameraRotation = glm::mat4_cast(rotation);
    glm::mat4 cameraTranslation = glm::translate(glm::mat4 { 1.0f }, position);
    return glm::inverse(cameraTranslation * cameraRotation);
}

vk::DescriptorSetLayout CameraResource::DescriptorSetLayout()
{
    return _descriptorSetLayout;
}
