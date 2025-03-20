#include "pathfinding_module.hpp"

#include "engine.hpp"
#include "model_loading.hpp"
#include "renderer_module.hpp"

#include "components/static_mesh_component.hpp"
#include "graphics_context.hpp"
#include "passes/debug_pass.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/model_resource_manager.hpp"
#include "scene/model_loader.hpp"

#include "scripting_module.hpp"

#include <set>

ModuleTickOrder PathfindingModule::Init(MAYBE_UNUSED Engine& engine)
{
    return ModuleTickOrder::eTick;
}

void PathfindingModule::Tick(MAYBE_UNUSED Engine& engine)
{
    // Draws any path that was generated through the scripting language
    if (_debugDraw)
    {
        _debugLines.clear();
        for (const auto& path : _computedPaths)
        {
            for (size_t i = 0; i < path.waypoints.size() - 1; i++)
            {
                glm::vec3 from = path.waypoints[i].centre;
                glm::vec3 to = path.waypoints[i + 1].centre;

                from += glm::vec3 { 0.0f, 0.1f, 0.0f };
                to += glm::vec3 { 0.0f, 0.1f, 0.0f };

                _debugLines.push_back(from);
                _debugLines.push_back(to);
            }
        }
    }

    //_computedPaths.clear();
}

void PathfindingModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}

PathfindingModule::PathfindingModule()
{
}

PathfindingModule::~PathfindingModule()
{
}

int32_t PathfindingModule::SetNavigationMesh(std::string_view filePath)
{
    CPUModel navmesh = ModelLoading::LoadGLTF(filePath);
    uint32_t meshIndex = std::numeric_limits<uint32_t>::max();
    glm::mat4 transform = glm::mat4(1.0f);

    const Node* rootNode = &navmesh.hierarchy.nodes[navmesh.hierarchy.root];

    std::queue<std::pair<const Node*, glm::mat4>> nodeStack;
    nodeStack.push({ rootNode, rootNode->transform });
    while (!nodeStack.empty())
    {
        const std::pair<const Node*, glm::mat4>& topNodeTransform = nodeStack.front();
        nodeStack.pop();

        if (topNodeTransform.first->mesh.has_value())
        {
            if (topNodeTransform.first->mesh.value().type == MeshType::eSTATIC)
            {
                meshIndex = topNodeTransform.first->mesh.value().index;
                transform = topNodeTransform.second;
                break;
            }
        }

        for (size_t i = 0; i < topNodeTransform.first->childrenIndices.size(); i++)
        {
            const Node* pNode = &navmesh.hierarchy.nodes[topNodeTransform.first->childrenIndices[i]];
            glm::mat4 transform = topNodeTransform.second * pNode->transform;

            nodeStack.push({ pNode, transform });
        }
    }

    if (meshIndex == std::numeric_limits<uint32_t>::max())
        return {};

    CPUMesh navmeshMesh = navmesh.meshes[meshIndex]; // GLTF model should consist of only a single mesh
    _mesh = navmeshMesh;
    _inverseTransform = glm::inverse(transform);

    _triangles.clear();

    std::unordered_map<uint32_t, std::vector<uint32_t>> indicesToTriangles;

    // We store all the triangles with their indices and center points
    //
    // We also store the triangles that use an index to find adjacent triangles
    for (size_t i = 0; i < navmeshMesh.indices.size(); i += 3)
    {
        TriangleInfo info {};
        info.indices[0] = navmeshMesh.indices[i];
        info.indices[1] = navmeshMesh.indices[i + 1];
        info.indices[2] = navmeshMesh.indices[i + 2];

        glm::vec3 p0 = navmeshMesh.vertices[info.indices[0]].position;
        glm::vec3 p1 = navmeshMesh.vertices[info.indices[1]].position;
        glm::vec3 p2 = navmeshMesh.vertices[info.indices[2]].position;

        info.centre = (p0 + p1 + p2) / 3.0f;
        info.centre = glm::vec3(transform * glm::vec4(info.centre, 1.0f));

        size_t triangleIdx = _triangles.size();
        _triangles.push_back(info);

        indicesToTriangles[info.indices[0]].push_back(triangleIdx);
        indicesToTriangles[info.indices[1]].push_back(triangleIdx);
        indicesToTriangles[info.indices[2]].push_back(triangleIdx);
    }

    // Loop over all triangles in the mesh
    for (size_t i = 0; i < _triangles.size(); i++)
    {
        TriangleInfo& triangle = _triangles[i];

        // We store all triangles that share indices with the current triangle
        // If a triangle shares two indices with the current triangle, it's adjacent to the current triangle
        std::unordered_multiset<uint32_t> sharedTriangles;

        // For each index in the current triangle
        for (uint32_t j = 0; j < 3; j++)
        {
            // Get all triangles that use this index
            const std::vector<uint32_t>& indexTriangles = indicesToTriangles[triangle.indices[j]];

            // Loop over every triangle that shares this index
            for (uint32_t triangleIdx : indexTriangles)
            {
                // Don't add current triangle to list, obviously
                if (triangleIdx == i)
                    continue;

                sharedTriangles.insert(triangleIdx);
            }
        }

        // Loop over all triangles that share indices with the current triangle
        for (const uint32_t& triangleIdx : sharedTriangles)
        {
            if (sharedTriangles.count(triangleIdx) > 1)
            {
                bool insert = true;
                // We share more than 2 indices now so we're adjacent to the current triangle
                for (uint32_t k = 0; k < triangle.adjacentTriangleCount; k++)
                {
                    if (triangle.adjacentTriangleIndices[k] == triangleIdx)
                    {
                        insert = false;
                    }
                }
                if (insert)
                {
                    triangle.adjacentTriangleIndices[triangle.adjacentTriangleCount++] = triangleIdx;
                }
            }
        }
    }

    // Now that we have adjacency information about the triangles we can start constructing a node tree
    for (size_t i = 0; i < _triangles.size(); i++)
    {
        TriangleInfo& info = _triangles[i];

        for (size_t j = 0; j < info.adjacentTriangleCount; j++)
        {
            _trianglesToNeighbours[i][j] = info.adjacentTriangleIndices[j];
        }
    }

    return 0;
}

ComputedPath PathfindingModule::FindPath(glm::vec3 startPos, glm::vec3 endPos)
{
    // Reference: https://app.datacamp.com/learn/tutorials/a-star-algorithm?dc_referrer=https%3A%2F%2Fwww.google.com%2F
    // Legend:
    // G = totalCost
    // H = estimateToGoal
    // F = totalEstimatedCost

    // Heuristic function
    IterablePriorityQueue<TriangleNode, std::vector<TriangleNode>, std::greater<TriangleNode>> openList;
    std::unordered_map<uint32_t, TriangleNode> closedList;

    // startPos = glm::vec3(_inverseTransform * glm::vec4(startPos, 1.0f));
    // endPos = glm::vec3(_inverseTransform * glm::vec4(endPos, 1.0f));

    // Find closest triangle // TODO: More efficient way of finding closes triangle
    float closestStartTriangleDistance = std::numeric_limits<float>::infinity(),
          closestDestinationTriangleDistance = std::numeric_limits<float>::infinity();
    uint32_t closestStartTriangleIndex = std::numeric_limits<uint32_t>::max(),
             closestDestinationTriangleIndex = std::numeric_limits<uint32_t>::max();
    for (size_t i = 0; i < _triangles.size(); i++)
    {
        float distance_to_start_triangle = glm::distance(startPos, _triangles[i].centre);
        float distance_to_finish_triangle = glm::distance(endPos, _triangles[i].centre);

        if (distance_to_start_triangle < closestStartTriangleDistance)
        {
            closestStartTriangleDistance = distance_to_start_triangle;
            closestStartTriangleIndex = i;
        }

        if (distance_to_finish_triangle < closestDestinationTriangleDistance)
        {
            closestDestinationTriangleDistance = distance_to_finish_triangle;
            closestDestinationTriangleIndex = i;
        }
    }

    TriangleNode node {};
    node.totalCost = 0;
    node.estimateToGoal = Heuristic(startPos, endPos);
    node.totalEstimatedCost = node.totalCost + node.estimateToGoal;
    node.triangleIndex = closestStartTriangleIndex;
    node.parentTriangleIndex = std::numeric_limits<uint32_t>::max();

    openList.push(node);

    while (!openList.empty())
    {
        TriangleNode topNode = openList.top();
        if (topNode.triangleIndex == closestDestinationTriangleIndex) // If we reached our goal
        {
            closedList[topNode.triangleIndex] = topNode;
            ComputedPath path = ReconstructPath(topNode.triangleIndex, closedList);
            _computedPaths.push_back(path);
            return path;
            break;
        }

        // Move current from open to closed list
        closedList[topNode.triangleIndex] = topNode;
        openList.pop();

        const TriangleInfo& triangleInfo = _triangles[topNode.triangleIndex];

        // For all this triangle's neighbours
        for (uint32_t i = 0; i < triangleInfo.adjacentTriangleCount; i++)
        {
            // Skip already evaluated nodes
            if (closedList.contains(triangleInfo.adjacentTriangleIndices[i]))
                continue;

            const TriangleInfo& neighbourTriangleInfo = _triangles[triangleInfo.adjacentTriangleIndices[i]];

            float tentativeCostFromStart = topNode.totalCost + glm::distance(triangleInfo.centre, neighbourTriangleInfo.centre);

            TriangleNode tempNode {};
            tempNode.triangleIndex = triangleInfo.adjacentTriangleIndices[i];
            auto it = openList.find(tempNode);
            if (it == openList.end())
            {
                openList.push(tempNode);
                it = openList.find(tempNode);
                it->totalCost = INFINITY;
            }

            TriangleNode& neighbourNode = *it;

            if (tentativeCostFromStart < neighbourNode.totalCost)
            {
                neighbourNode.totalCost = tentativeCostFromStart;
                neighbourNode.estimateToGoal = Heuristic(neighbourTriangleInfo.centre, endPos);
                neighbourNode.parentTriangleIndex = topNode.triangleIndex;
                neighbourNode.totalEstimatedCost = neighbourNode.totalCost + neighbourNode.estimateToGoal;
            }
        }
    }

    return {};
}

float PathfindingModule::Heuristic(glm::vec3 startPos, glm::vec3 endPos)
{
    return glm::length(endPos - startPos);
}

ComputedPath PathfindingModule::ReconstructPath(const uint32_t finalTriangleIndex, std::unordered_map<uint32_t, TriangleNode>& nodes)
{
    ComputedPath path = {};

    PathNode pathNode {};

    const TriangleNode& finalTriangleNode = nodes[finalTriangleIndex];

    pathNode.centre = _triangles[finalTriangleIndex].centre;
    path.waypoints.push_back(pathNode);

    uint32_t previousTriangleIndex = finalTriangleIndex;
    uint32_t parentTriangleIndex = finalTriangleNode.parentTriangleIndex;

    while (true)
    {
        if (parentTriangleIndex == std::numeric_limits<uint32_t>::max())
        {
            pathNode.centre = _triangles[previousTriangleIndex].centre;
            path.waypoints.push_back(pathNode);
            break;
        }

        const TriangleNode& node = nodes[parentTriangleIndex];
        // pathNode.centre = _triangles[parentTriangleIndex].centre;
        // path.waypoints.push_back(pathNode);

        std::unordered_map<uint32_t, uint32_t> triangleIndices;
        for (int32_t i = 0; i < 3; i++)
        {
            triangleIndices[_triangles[previousTriangleIndex].indices[i]]++;
        }
        for (int32_t i = 0; i < 3; i++)
        {
            triangleIndices[_triangles[parentTriangleIndex].indices[i]]++;
        }

        uint32_t adjacentEdgeIndices[2] = {};

        uint32_t i = 0;
        for (const auto& [index, count] : triangleIndices)
        {
            if (count == 2)
            {
                adjacentEdgeIndices[i++] = index;
            }
        }

        pathNode.centre = (_mesh.vertices[adjacentEdgeIndices[0]].position + _mesh.vertices[adjacentEdgeIndices[1]].position) * 0.5f;
        pathNode.centre = glm::vec3(glm::inverse(_inverseTransform) * glm::vec4(pathNode.centre, 1.0f));

        path.waypoints.push_back(pathNode);

        previousTriangleIndex = parentTriangleIndex;
        parentTriangleIndex = node.parentTriangleIndex;
    }

    std::reverse(path.waypoints.begin(), path.waypoints.end());

    return path;
}
