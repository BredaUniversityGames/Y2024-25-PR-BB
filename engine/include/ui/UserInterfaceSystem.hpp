//
// Created by luuk on 16-9-2024.
//

#pragma once
#include "typeindex"
#include <pch.hpp>
#include <queue>

#include "../input_manager.hpp"
#include "../../../external/entt-master/src/entt/entt.hpp"
#include "../pipelines/generic_pipeline.h"
#include <expected>

class UserInterfaceRenderContext;
class GenericPipeline;
struct CameraUBO;
struct Camera;
class SwapChain;

class UIPipeLine;

class UIRenderSystemBase
{
public:
    UIRenderSystemBase(const UIPipeLine& pipe_line)
        : m_PipeLine(pipe_line)
    {
    }

    virtual void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix) = 0;

    virtual ~UIRenderSystemBase() = default;

protected:
    const UIPipeLine& m_PipeLine;
};

template <typename T>
class UIRenderSystem : public UIRenderSystemBase
{
public:
    UIRenderSystem(const UIPipeLine& pipe_line)
        : UIRenderSystemBase(pipe_line)
    {
    }
    std::queue<T> renderQueue;

    virtual ~UIRenderSystem() = default;
};

/**
 * Base class from which all ui elements inherit from.Updating and rendering of the ui happens
 * mostly in a hierarchical manner. each element calls its children's update and draw functions.
 * class contains pure virtual functions and is thus abstract.
 */

struct UIElement
{
    enum class AnchorPoint
    {
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT
    };

    void SetLocation(const glm::vec2& location) { RelativeLocation = location; }

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     */
    void UpdateAbsoluteLocation(const glm::vec2& location) { AbsoluteLocation = location; }

    /**
     *
     * @return the location of the element relative to the set anchorpoint of the parent element.
     */
    [[nodiscard]] const glm::vec2& GetRelativeLocation() const { return RelativeLocation; }

    /**
     * submits drawinfo to the appropriate rendering system inside the current UserInterfaceContext.
     */
    virtual void SubmitDrawInfo(UserInterfaceRenderContext&) const
    {
    }

    virtual void Update(const InputManager&)
    {
    }

    AnchorPoint m_AnchorPoint = AnchorPoint::TOP_LEFT;
    bool m_Visible = true;

    virtual void UpdateChildAbsoluteLocations() = 0;
    glm::vec2 Scale {};

protected:
    glm::vec2 AbsoluteLocation {};
    glm::vec2 RelativeLocation {};

    std::vector<std::unique_ptr<UIElement>> chilren;
};

void UpdateUI(const InputManager& input, UIElement* element);

void RenderUI(UIElement* element, UserInterfaceRenderContext& context, const vk::CommandBuffer&, const VulkanBrain&, ResourceHandle<Image>& renderTarget, const glm::mat4& projectionMatrix);

/**
 * holds free floating elements. elements can be anchored to one of the 4 corners of the canvas. anchors help preserve
 * the layout across different resolutions.
 */
struct Canvas : public UIElement
{
    void UpdateChildAbsoluteLocations() override;
};

/**
 *  main class responsable for the updating and rendering of the UI. Does not hold any data on its own and just applies
 *  logic on the passed registry. By default, this object is contained in the Engine class and persists throughout
 *  the program.
 */
class UserInterfaceRenderContext
{
public:
    /**
     * Initialises and adds the default UI subsystems. these include:
     *
     *	-UIButtonSubSystem
     *	-UIDisplayTextSubSystem
     *
     * Note that calling this function is optional and if you do not want or need these systems
      this function call can be omitted.
     */
    void InitializeDefaultRenderSystems(const UIPipeLine& pipeline);

    /**
     *
     * @tparam T render system type, must be derived from UIRenderSystemBase.
     * @return If operation was successful.
     */
    template <typename T>
    bool AddRenderingSystem(const UIPipeLine& pipe_line)
    {
        static_assert(std::is_base_of<UIRenderSystemBase, T>::value,
            "Subsystem must be derived from UIRenderSystemBase");

        if (!m_UIRenderSystems.contains(typeid(T))) [[likely]]
        {
            auto result = m_UIRenderSystems.emplace(typeid(T), std::make_unique<T>(pipe_line));
            return true;
        }

        // todo: only log when logging is enabled.
        spdlog::warn("UIRenderSystem {} already exists, cannot add again", typeid(T).name());
        return false;
    }

    template <typename T>
    T& GetRenderingSystem()
    {
        return (static_cast<T*>(m_UIRenderSystems.at(std::type_index(typeid(T))).get()));
    }

    std::unordered_map<std::type_index, std::unique_ptr<UIRenderSystemBase>> m_UIRenderSystems;

protected:
};

// todo: refactor
class UIPipeLine
{
public:
    void CreatePipeLine();
    UIPipeLine(const VulkanBrain& brain, const SwapChain& sc)
        : m_brain(brain)
        , m_swapChain(sc) {};

    void CreateDescriptorSetLayout();
    void UpdateTexture(ResourceHandle<Image> image, vk::DescriptorSet& set) const;
    void RecordCommands();
    NON_COPYABLE(UIPipeLine);
    NON_MOVABLE(UIPipeLine);
    ~UIPipeLine();

    VkPipeline m_uiPipeLine;
    vk::PipelineLayout m_pipelineLayout;
    vk::DescriptorSet m_descriptorSet {};
    static vk::DescriptorSetLayout m_descriptorSetLayout;
    const VulkanBrain& m_brain;
    const SwapChain& m_swapChain;
    vk::UniqueSampler m_sampler;
};
