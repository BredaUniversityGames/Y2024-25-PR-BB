#include "components/skeleton_component.hpp"

void SkeletonHelpers::AttachChild(entt::registry& registry, entt::entity parent, entt::entity child)
{
    auto& parentNode = registry.get<SkeletonNodeComponent>(parent);
    auto& childNode = registry.get<SkeletonNodeComponent>(child);

    childNode.parent = parent;
    auto firstChildIt = std::find_if(parentNode.children.begin(), parentNode.children.end(), [](const auto& element)
        { return element == entt::null; });

    assert(firstChildIt != parentNode.children.end());

    *firstChildIt = child;
}

void SkeletonHelpers::InitializeSkeletonNode(SkeletonNodeComponent& node)
{
    node.parent = entt::null;
    for (auto& child : node.children)
    {
        child = entt::null;
    }
}
