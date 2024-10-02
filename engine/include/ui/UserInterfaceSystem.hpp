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

    virtual void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain&) = 0;

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

    explicit UIElement(uint16_t max_children)
        : m_MaxChildren(max_children)
    {
    }

    enum class AnchorPoint
    {
        MIDDLE,
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT,
    };

    void SetLocation(const glm::vec2& location) { RelativeLocation = location; }

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     */
    void UpdateAbsoluteLocation(const glm::vec2& location, bool updateChildren = true)
    {
        AbsoluteLocation = location;
        if (updateChildren)
            UpdateChildAbsoluteLocations();
    }

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

    virtual void Update(const InputManager& input)
    {
        for (auto& i : chilren)
            i->Update(input);
    }

    void AddChild(std::unique_ptr<UIElement> child)
    {
        if (chilren.size() < m_MaxChildren && child != nullptr)
            chilren.push_back(std::move(child));
        else
            spdlog::warn("UIElement::AddChild:Can't add, Too many children");
    }

    [[nodiscard]] const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
    {
        return chilren;
    }

    AnchorPoint m_AnchorPoint
        = AnchorPoint::TOP_LEFT;

    bool m_Visible = true;

    virtual void UpdateChildAbsoluteLocations() = 0;

    glm::vec2 Scale {};

    virtual ~UIElement() = default;

protected:
    glm::vec2 AbsoluteLocation {};
    glm::vec2 RelativeLocation {};

private:
    uint16_t m_MaxChildren = 0;
    std::vector<std::unique_ptr<UIElement>> chilren;
};

void UpdateUI(const InputManager& input, UIElement* element);

void RenderUI(UIElement* element, UserInterfaceRenderContext& context, const vk::CommandBuffer&, const VulkanBrain&, SwapChain& swapChain, int swapChainIndex, const glm::mat4& projectionMatrix);

/**
 * holds free floating elements. elements can be anchored to one of the 4 corners of the canvas. anchors help preserve
 * the layout across different resolutions.
 */
struct Canvas : public UIElement
{
public:
    void UpdateChildAbsoluteLocations() override;
    void SubmitDrawInfo(UserInterfaceRenderContext&) const override;
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
     * @tparam T Render system type, must be derived from UIRenderSystemBase.
     * @return If the operation was successful.
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
    T* GetRenderingSystem()
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
