#pragma once

class BloomSettings
{
public:
    struct alignas(16) SettingsData
    {
        // How much brightness is extracted from each color channel.
        glm::vec3 colorWeights = glm::vec3(0.2126f, 0.7152f, 0.0722f);

        // Overall strength of the final output.
        float strength = 0.8f;

        // How strong the brightness difference should be between dark and bright spots. The higher this is the stronger specular reflections will be.
        float gradientStrength = 0.2f;

        // The maximum amount of brightness that can be extracted per pixel.
        float maxBrightnessExtraction = 5.0f;

    private:
        glm::vec2 _padding;
    };

    struct FrameData
    {
        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> buffers;
    };

    BloomSettings(const VulkanBrain& brain);
    ~BloomSettings();
    void Render();
    void Update(uint32_t currentFrame);
    const vk::DescriptorSet& GetDescriptorSetData(uint32_t currentFrame) const { return _frameData.descriptorSets[currentFrame]; }
    const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return _descriptorSetLayout; }

    SettingsData _data;

private:
    const VulkanBrain& _brain;
    vk::DescriptorSetLayout _descriptorSetLayout;
    FrameData _frameData;

    void CreateDescriptorSetLayout();
    void CreateUniformBuffers();
    void UpdateDescriptorSet(uint32_t currentFrame);
};
