#include "bloom_settings.hpp"

#include <imgui.h>

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "settings.hpp"
#include "vulkan_helper.hpp"

BloomSettings::BloomSettings(const std::shared_ptr<GraphicsContext>& context, const Settings::Bloom& settings)
    : _context(context)
    , _settings(settings)
{
    CreateDescriptorSetLayout();
    CreateUniformBuffers();
}

BloomSettings::~BloomSettings()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    vkContext->Device().destroy(_descriptorSetLayout);
}

void BloomSettings::Update(uint32_t currentFrame)
{
    _data.strength = _settings.strength;
    _data.colorWeights = _settings.colorWeights;
    _data.gradientStrength = _settings.gradientStrength;
    _data.maxBrightnessExtraction = _settings.maxBrightnessExtraction;
    _data.filterRadius = _settings.filterRadius;

    auto resources { _context->Resources() };

    const Buffer* buffer = resources->BufferResourceManager().Access(_frameData.buffers.at(currentFrame));
    memcpy(buffer->mappedPtr, &_data, sizeof(SettingsData));
}

void BloomSettings::CreateDescriptorSetLayout()
{
    auto vkContext { _context->VulkanContext() };

    std::vector<vk::DescriptorSetLayoutBinding> bindings {};
    bindings.emplace_back(vk::DescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute,
    });
    std::vector<std::string_view> names { "BloomSettingsUBO" };

    _descriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
    util::NameObject(_descriptorSetLayout, "Bloom settings DSL", vkContext);
}

void BloomSettings::CreateUniformBuffers()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        std::string name = "[] Bloom settings UBO";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(FrameData))
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer)
            .SetName(name);

        _frameData.buffers.at(i) = resources->BufferResourceManager().Create(creation);
    }

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = vkContext->DescriptorPool();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, _frameData.descriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < _frameData.descriptorSets.size(); ++i)
    {
        UpdateDescriptorSet(i);
    }
}

void BloomSettings::UpdateDescriptorSet(uint32_t currentFrame)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    const Buffer* buffer = resources->BufferResourceManager().Access(_frameData.buffers.at(currentFrame));

    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(SettingsData);

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _frameData.descriptorSets.at(currentFrame);
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
