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

glm::vec3 Follow(ComputedPath& path, uint32_t current_index)
{
    assert(path.waypoints.size() > 0);

    auto& p1 = path.waypoints.at(current_index);

    if (current_index + 1 < path.waypoints.size())
    {
        auto& p2 = path.waypoints.at(current_index + 1);
        float dst = glm::distance(p1.centre, p2.centre);
        return glm::mix(p1.centre, p2.centre, dst * 0.03f);
    }
    else
    {
        return p1.centre;
    }
}

// 0 = no skip, 1 = skip, 2 = path over
uint32_t ShouldGoNextWaypoint(ComputedPath& path, uint32_t current_index, const glm::vec3& position)
{
    assert(path.waypoints.size() > 0);

    if (glm::distance(position, path.waypoints.at(current_index).centre) < 3.0f)
    {
        if (current_index + 1 == path.waypoints.size())
        {
            // If we are at the last waypoint, we should end the path
            return 2;
        }

        return 1;
    }
    else
    {
        return 0;
    }
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

    computedPath.funcExt<bindings::GetSize>("Count");
    computedPath.funcExt<bindings::Follow>("GetFollowDirection");
    computedPath.funcExt<bindings::ShouldGoNextWaypoint>("ShouldGoNextWaypoint");
    computedPath.funcExt<bindings::ClearWaypoints>("ClearWaypoints");
    computedPath.funcExt<bindings::GetWaypoint>("GetWaypoint");
}
