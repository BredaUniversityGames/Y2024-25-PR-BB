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

void ToggleDebugRender(PathfindingModule& self)
{
    self.SetDebugDrawState(!self.GetDebugDrawState());
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
}
