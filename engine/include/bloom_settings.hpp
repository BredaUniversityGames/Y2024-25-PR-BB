#pragma once

class BloomSettings
{
public:
    struct SettingsData
    {
        /// Overall strength of the final output.
        float strength = 0.8f;

        /// How strong the brightness difference should be between dark and bright spots. The higher this is the stronger specular reflections will be.
        float gradientStrength = 0.2f;
    };

    struct FrameData
    {
        vk::DescriptorSetLayout descriptorSetLayout;
        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> buffers;
        std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> allocations;
        std::array<void*, MAX_FRAMES_IN_FLIGHT> mappedPtrs;
    };

    BloomSettings(const VulkanBrain& brain);
    ~BloomSettings();
    void Update(uint32_t currentFrame);
    const vk::DescriptorSet& GetDescriptorSetData(uint32_t currentFrame) const { return _frameData.descriptorSets[currentFrame]; }
    const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return _frameData.descriptorSetLayout; }

    SettingsData _data;

private:
    const VulkanBrain& _brain;
    vk::DescriptorSetLayout _descriptorSetLayout;
    FrameData _frameData;

    void CreateDescriptorSetLayout();
    void CreateUniformBuffers();
    void UpdateDescriptorSet(uint32_t currentFrame);
};
