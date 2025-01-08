#include "pathfinding_module.hpp"

#include "engine.hpp"
#include "model_loader.hpp"
#include "renderer_module.hpp"
#include <set>

ModuleTickOrder PathfindingModule::Init(MAYBE_UNUSED Engine& engine)
{
    _renderer = engine.GetModule<RendererModule>().GetRenderer();

    this->SetNavigationMesh("assets/models/NavmeshTest/NavMesh.gltf");
    ComputedPath path = this->FindPath(
        { 0.7f, 0.0f, -0.7f },
        { -0.7f, 0.0f, 0.7f });

    return ModuleTickOrder::eTick;
}

void PathfindingModule::Tick(MAYBE_UNUSED Engine& engine)
{
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

int32_t PathfindingModule::SetNavigationMesh(const std::string& mesh_path)
{
    CPUModel navmesh = _renderer->GetModelLoader().ExtractModelFromGltfFile(mesh_path);
    CPUMesh navmeshMesh = navmesh.meshes[0]; // GLTF model should consist of only a single mesh

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
            const std::vector<uint32_t>& index_triangles = indicesToTriangles[triangle.indices[j]];

            // Loop over every triangle that shares this index
            for (uint32_t triangleIdx : index_triangles)
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
            return ReconstructPath(topNode.triangleIndex, closedList);
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

    uint32_t parentTriangleIndex = finalTriangleNode.parentTriangleIndex;

    while (true)
    {
        if (parentTriangleIndex == std::numeric_limits<uint32_t>::max())
            break;

        const TriangleNode& node = nodes[parentTriangleIndex];
        pathNode.centre = _triangles[parentTriangleIndex].centre;
        path.waypoints.push_back(pathNode);

        parentTriangleIndex = node.parentTriangleIndex;
    }

    return path;
}