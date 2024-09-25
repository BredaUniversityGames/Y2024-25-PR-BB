#include "bloom_settings.hpp"
#include "vulkan_helper.hpp"

BloomSettings::BloomSettings(const VulkanBrain& brain)
    :
    _brain(brain)
{
    CreateDescriptorSetLayout();
    CreateUniformBuffers();
}

BloomSettings::~BloomSettings()
{
    _brain.device.destroy(_frameData.descriptorSetLayout);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _frameData.allocations[i]);
        vmaDestroyBuffer(_brain.vmaAllocator, _frameData.buffers[i], _frameData.allocations[i]);
    }
}

void BloomSettings::Update(uint32_t currentFrame)
{
    ImGui::Begin("Bloom Settings");

    ImGui::InputFloat("Strength", &_data.strength, 0.05f, 0.1f);
    ImGui::InputFloat("Gradient strength", &_data.gradientStrength, 0.05f, 0.1f, "%.00005f");
    ImGui::InputFloat3("Color weights", &_data.colorWeights[0], "%.00005f");

    ImGui::End();

    memcpy(_frameData.mappedPtrs[currentFrame], &_data, sizeof(SettingsData));
}

void BloomSettings::CreateDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding descriptorSetBinding {};
    descriptorSetBinding.binding = 0;
    descriptorSetBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorSetBinding.descriptorCount = 1;
    descriptorSetBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &descriptorSetBinding;
    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_frameData.descriptorSetLayout),
        "Failed creating bloom settings UBO descriptor set layout!");
}

void BloomSettings::CreateUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::CreateBuffer(_brain, sizeof(FrameData),
            vk::BufferUsageFlagBits::eUniformBuffer,
            _frameData.buffers[i], true, _frameData.allocations[i],
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Bloom settings uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _frameData.allocations[i], &_frameData.mappedPtrs[i]), "Failed mapping memory for UBO!");
    }

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _frameData.descriptorSetLayout; });
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
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = _frameData.buffers[currentFrame];
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