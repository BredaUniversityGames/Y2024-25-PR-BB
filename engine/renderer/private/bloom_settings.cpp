#include "bloom_settings.hpp"
#include "imgui/imgui.h"
#include "pipeline_builder.hpp"
#include "vulkan_helper.hpp"

BloomSettings::BloomSettings(const VulkanContext& brain)
    : _brain(brain)
{
    CreateDescriptorSetLayout();
    CreateUniformBuffers();
}

BloomSettings::~BloomSettings()
{
    _brain.device.destroy(_descriptorSetLayout);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _brain.GetBufferResourceManager().Destroy(_frameData.buffers[i]);
    }
}

void BloomSettings::Render()
{
    ImGui::Begin("Bloom Settings");

    ImGui::InputFloat("Strength", &_data.strength, 0.5f, 2.0f);
    ImGui::InputFloat("Gradient strength", &_data.gradientStrength, 0.05f, 0.1f, "%.00005f");
    ImGui::InputFloat("Max brightness extraction", &_data.maxBrightnessExtraction, 1.0f, 5.0f);
    ImGui::InputFloat3("Color weights", &_data.colorWeights[0], "%.00005f");

    ImGui::End();
}

void BloomSettings::Update(uint32_t currentFrame)
{
    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_frameData.buffers[currentFrame]);
    memcpy(buffer->mappedPtr, &_data, sizeof(SettingsData));
}

void BloomSettings::CreateDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings {};
    bindings.emplace_back(vk::DescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute,
    });
    std::vector<std::string_view> names { "BloomSettingsUBO" };

    _descriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(_brain, bindings, names);
    util::NameObject(_descriptorSetLayout, "Bloom settings DSL", _brain);
}

void BloomSettings::CreateUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        std::string name = "[] Bloom settings UBO";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(FrameData))
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer)
            .SetName(name);

        _frameData.buffers[i] = _brain.GetBufferResourceManager().Create(creation);
    }

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, _frameData.descriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < _frameData.descriptorSets.size(); ++i)
    {
        UpdateDescriptorSet(i);
    }
}

void BloomSettings::UpdateDescriptorSet(uint32_t currentFrame)
{
    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_frameData.buffers[currentFrame]);

    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(SettingsData);

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _frameData.descriptorSets[currentFrame];
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}