#pragma once

#include "cpu_resources.hpp"
#include "module_interface.hpp"
#include "renderer.hpp"
#include <glm/glm.hpp>
#include <queue>
#include <unordered_map>

struct PathNode
{
    glm::vec3 centre;
};

struct ComputedPath
{
    ComputedPath() = default;
    ~ComputedPath() = default;

    std::vector<PathNode> waypoints;
};

struct TriangleNode
{
    float totalCost;
    float estimateToGoal;
    float totalEstimatedCost;
    uint32_t triangleIndex;
    uint32_t parentTriangleIndex;

    bool operator<(const TriangleNode& rhs)
    {
        return this->totalCost < rhs.totalCost;
    }
    bool operator>(const TriangleNode& rhs)
    {
        return this->totalCost > rhs.totalCost;
    }

    bool operator==(const TriangleNode& rhs)
    {
        return this->triangleIndex == rhs.triangleIndex;
    }
};

inline bool operator<(const TriangleNode& lhs, const TriangleNode& rhs)
{
    return lhs.totalCost < rhs.totalCost;
}

inline bool operator>(const TriangleNode& lhs, const TriangleNode& rhs)
{
    return lhs.totalCost > rhs.totalCost;
}

inline bool operator==(const TriangleNode& lhs, const TriangleNode& rhs)
{
    return lhs.triangleIndex == rhs.triangleIndex;
}

template <class _Ty, class _Container = std::vector<_Ty>, class _Pr = std::less<typename _Container::value_type>>
struct IterablePriorityQueue : public std::priority_queue<_Ty, _Container, _Pr>
{
public:
    typename std::vector<_Ty>::iterator begin()
    {
        return this->c.begin();
    }

    typename std::vector<_Ty>::iterator end()
    {
        return this->c.end();
    }

    typename _Container::iterator find(const _Ty& item)
    {
        return std::find(this->c.begin(), this->c.end(), item);
    }
};

class PathfindingModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;
    std::string_view GetName() final { return "Pathfinding"; };

public:
    PathfindingModule();
    ~PathfindingModule();

    NON_COPYABLE(PathfindingModule)
    NON_MOVABLE(PathfindingModule)

    int32_t SetNavigationMesh(std::string_view filePath);
    ComputedPath FindPath(glm::vec3 startPos, glm::vec3 endPos);

    bool SetDebugDrawState(bool state) { return _debugDraw = state; }
    bool GetDebugDrawState() const { return _debugDraw; }

    const std::vector<glm::vec3>& GetDebugLines() const { return _debugLines; }

private:
    float Heuristic(glm::vec3 startPos, glm::vec3 endPos);
    float DirectedHeuristic(glm::vec3 startPos, glm::vec3 endPos, glm::vec3 finalPosition);
    float TiebreakerHeuristic(glm::vec3 startpos, glm::vec3 currentPos, glm::vec3 goalPosition);
    ComputedPath ReconstructPath(const uint32_t finalTriangleIndex, std::unordered_map<uint32_t, TriangleNode>& nodes);

    struct TriangleInfo
    {
        uint32_t indices[3] = {
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max()
        };

        glm::vec3 centre = glm::vec3 { 0.0f };

        uint32_t adjacentTriangleIndices[3] = {
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max()
        };
        uint8_t adjacentTriangleCount = 0;
    };

    std::vector<TriangleInfo> _triangles;
    std::unordered_map<uint32_t, uint32_t[3]> _trianglesToNeighbours;
    glm::mat4 _inverseTransform = glm::mat4(1.0f);

    CPUMesh<Vertex> _mesh = {};

    bool _debugDraw = false;
    std::vector<glm::vec3> _debugLines;
    std::vector<ComputedPath> _computedPaths;
};
