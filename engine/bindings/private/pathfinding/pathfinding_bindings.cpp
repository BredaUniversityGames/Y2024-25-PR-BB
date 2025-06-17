#include "pathfinding_bindings.hpp"

#include "pathfinding_module.hpp"

#include <glm/glm.hpp>

namespace bindings
{
int32_t SetNavigationMesh(PathfindingModule& self, const std::string& path)
{
    return self.SetNavigationMesh(path);
}

ComputedPath FindPath(PathfindingModule& self, const glm::vec3& start_pos, const glm::vec3& end_pos)
{
    return self.FindPath(start_pos, end_pos);
}

glm::vec3 GetCenter(PathNode& node)
{
    return node.centre;
}

const std::vector<PathNode>& GetWaypoints(ComputedPath& path)
{
    return path.waypoints;
}

PathNode GetWaypoint(ComputedPath& path, uint32_t index)
{
    return path.waypoints.at(index);
}

uint32_t GetSize(ComputedPath& path)
{
    return path.waypoints.size();
}

void ClearWaypoints(ComputedPath& path)
{
    path.waypoints.clear();
}

void ToggleDebugRender(PathfindingModule& self)
{
    self.SetDebugDrawState(!self.GetDebugDrawState());
}

glm::vec3 Follow(ComputedPath& path, const glm::vec3& currentPos, uint32_t current_index)
{
    assert(path.waypoints.size() > 0);

    auto& p1 = path.waypoints.at(current_index);

    if (current_index + 1 < path.waypoints.size())
    {
        auto& p2 = path.waypoints.at(current_index + 1);
        auto dst = glm::distance(currentPos, p1.centre);
        auto target = glm::mix(p1.centre, p2.centre, glm::clamp(dst * 0.5f, 0.0f, 1.0f));
        return glm::normalize(target - currentPos);
    }
    else
    {
        return glm::normalize(p1.centre - currentPos);
    }
}

bool ShouldGoNextWaypoint(ComputedPath& path, uint32_t current_index, const glm::vec3& position, float bias)
{
    assert(path.waypoints.size() > 0);

    bool shouldGoNext = glm::distance(position, path.waypoints.at(current_index).centre) < bias;
    return shouldGoNext;
}
}

void BindPathfindingAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<PathfindingModule>("PathfindingModule");
    wren_class.funcExt<bindings::ToggleDebugRender>("ToggleDebugRender");

    wren_class.funcExt<bindings::FindPath>("FindPath");
    wren_class.funcExt<bindings::SetNavigationMesh>("SetNavigationMesh");

    auto& pathNode = module.klass<PathNode>("PathNode");
    pathNode.propReadonlyExt<bindings::GetCenter>("center");

    auto& computedPath = module.klass<ComputedPath>("ComputedPath");

    computedPath.funcExt<bindings::GetWaypoints>("GetWaypoints");
    computedPath.funcExt<bindings::GetSize>("Count");
    computedPath.funcExt<bindings::Follow>("GetFollowDirection");
    computedPath.funcExt<bindings::ShouldGoNextWaypoint>("ShouldGoNextWaypoint");
    computedPath.funcExt<bindings::ClearWaypoints>("ClearWaypoints");
    computedPath.funcExt<bindings::GetWaypoint>("GetWaypoint");
}
