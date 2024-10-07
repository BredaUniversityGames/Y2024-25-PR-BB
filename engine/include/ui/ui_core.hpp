//
// Created by luuk on 16-9-2024.
//

#pragma once
#include "typeindex"
#include <pch.hpp>
#include <queue>

#include "../input_manager.hpp"
#include <expected>

class UserInterfaceRenderer;
class GenericPipeline;
struct CameraUBO;
struct Camera;
class SwapChain;

class UIPipeLine;

class UIRenderSystemBase
{
public:
    UIRenderSystemBase(std::shared_ptr<UIPipeLine>& pipe_line)
        : m_PipeLine(pipe_line)
    {
    }

    virtual void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain&) = 0;

    virtual ~UIRenderSystemBase() = default;

protected:
    std::shared_ptr<UIPipeLine> m_PipeLine;
};

template <typename T>
class UIRenderSystem : public UIRenderSystemBase
{
public:
    UIRenderSystem(std::shared_ptr<UIPipeLine>& pl)
        : UIRenderSystemBase(pl)
    {
    }
    std::queue<T> renderQueue;

    virtual ~UIRenderSystem() = default;
};

/**
 * Base class from which all ui elements inherit from. Updating and rendering of the ui happens
 * mostly in a hierarchical manner. each element calls its children's update and draw functions.
 * class contains pure virtual functions.
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

    void SetLocation(const glm::vec2& location) { m_RelativeLocation = location; }

    /**
     * note: mostly for internal use to calculate the correct screen space position based on it's parents.
     * @param location new location
     */
    void UpdateAbsoluteLocation(const glm::vec2& location, bool updateChildren = true)
    {
        m_AbsoluteLocation = location;
        if (updateChildren)
            UpdateChildAbsoluteLocations();
    }

    /**
     *
     * @return the location of the element relative to the set anchorpoint of the parent element.
     */
    [[nodiscard]] const glm::vec2& GetRelativeLocation() const { return m_RelativeLocation; }

    /**
     * submits drawinfo to the appropriate rendering system inside the current UserInterfaceContext.
     */
    virtual void SubmitDrawInfo(UserInterfaceRenderer&) const
    {
    }

    virtual void Update(const InputManager& input)
    {
        for (auto& i : m_Children)
            i->Update(input);
    }

    void AddChild(std::unique_ptr<UIElement> child)
    {
        if (m_Children.size() < m_MaxChildren && child != nullptr)
            m_Children.push_back(std::move(child));
        else
            spdlog::warn("UIElement::AddChild:Can't add, Too many children");
    }

    [[nodiscard]] const std::vector<std::unique_ptr<UIElement>>& GetChildren() const
    {
        return m_Children;
    }

    AnchorPoint m_AnchorPoint
        = AnchorPoint::TOP_LEFT;

    bool m_Visible = true;

    virtual void UpdateChildAbsoluteLocations() = 0;

    glm::vec2 m_Scale {};

    virtual ~UIElement() = default;

protected:
    glm::vec2 m_AbsoluteLocation {};
    glm::vec2 m_RelativeLocation {};

private:
    uint16_t m_MaxChildren = 0;
    std::vector<std::unique_ptr<UIElement>> m_Children {};
};

void UpdateUI(const InputManager& input, UIElement* element);

void RenderUI(UIElement* element, UserInterfaceRenderer& context, const vk::CommandBuffer&, const VulkanBrain&, SwapChain& swapChain, int swapChainIndex);

/**
 * holds free floating elements. elements can be anchored to one of the 4 corners of the canvas. anchors help preserve
 * the layout across different resolutions.
 */
struct Canvas : public UIElement
{
public:
    Canvas()
        : UIElement(std::numeric_limits<uint16_t>::max())
    {
    }
    void UpdateChildAbsoluteLocations() override;
    void SubmitDrawInfo(UserInterfaceRenderer&) const override;
};

class UserInterfaceRenderer
{
public:
    explicit UserInterfaceRenderer(const VulkanBrain& b)
        : m_VulkanBrain(b)
    {
    }

    /**
     * Initialises and adds the default UI subsystems. these include:
     *
     *	-UIButtonSubSystem
     *	-UIDisplayTextSubSystem
     *
     * Note that calling this function is optional and if you do not want or need these systems
      this function call can be omitted.
     */
    void InitializeDefaultRenderSystems();

    /**
     *
     * @tparam T Render system type, must be derived from UIRenderSystemBase.
     * @return If the operation was successful.
     */
    template <typename T>
    bool AddRenderingSystem(std::shared_ptr<UIPipeLine>& pipe_line)
    {
        static_assert(std::is_base_of<UIRenderSystemBase, T>::value,
            "Subsystem must be derived from UIRenderSystemBase");

        if (!m_UIRenderSystems.contains(typeid(T))) [[likely]]
        {
            m_UIRenderSystems.emplace(typeid(T), std::make_unique<T>(pipe_line));
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
    const VulkanBrain& m_VulkanBrain;
};
