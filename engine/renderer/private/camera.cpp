#include "camera.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <glm/gtc/quaternion.hpp>

vk::DescriptorSetLayout CameraResource::_descriptorSetLayout;

CameraResource::CameraResource(const std::shared_ptr<GraphicsContext>& context)
    : _context(context)
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
    return p / length(glm::vec3(p));
}

void CameraResource::Update(uint32_t currentFrame, const Camera& camera)
{
    GPUCamera cameraBuffer {};

    glm::mat4 cameraRotation = glm::mat4_cast(glm::quat(camera.eulerRotation));
    glm::mat4 cameraTranslation = glm::translate(glm::mat4 { 1.0f }, camera.position);

    cameraBuffer.view = glm::inverse(cameraTranslation * cameraRotation);

    switch (camera.projection)
    {
    case Camera::Projection::ePerspective:
    {
        cameraBuffer.proj = glm::perspective(camera.fov, camera.aspectRatio, camera.nearPlane, camera.farPlane);

        glm::mat4 projT = glm::transpose(cameraBuffer.proj);

        glm::vec4 frustumX = normalizePlane(projT[3] + projT[0]);
        glm::vec4 frustumY = normalizePlane(projT[3] + projT[1]);

        cameraBuffer.frustum[0] = frustumX.x;
        cameraBuffer.frustum[1] = frustumX.z;
        cameraBuffer.frustum[2] = frustumY.y;
        cameraBuffer.frustum[3] = frustumY.z;
    }
    break;
    case Camera::Projection::eOrthographic:
    {
        float left = -camera.orthographicSize;
        float right = camera.orthographicSize;
        float bottom = -camera.orthographicSize;
        float top = camera.orthographicSize;
        cameraBuffer.proj = glm::ortho<float>(left, right, bottom, top, camera.nearPlane, camera.farPlane);

        cameraBuffer.frustum[0] = left;
        cameraBuffer.frustum[1] = right;
        cameraBuffer.frustum[2] = bottom;
        cameraBuffer.frustum[3] = top;
    }
    break;
    }
    cameraBuffer.proj[1][1] *= -1;
    cameraBuffer.projectionType = static_cast<int32_t>(camera.projection);

    cameraBuffer.VP = cameraBuffer.proj * cameraBuffer.view;
    cameraBuffer.cameraPosition = camera.position;

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

vk::DescriptorSetLayout CameraResource::DescriptorSetLayout()
{
    return _descriptorSetLayout;
}
