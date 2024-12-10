#pragma once

#include "module_interface.hpp"
#include "renderer.hpp"
#include <glm/glm.hpp>

struct ComputedPath
{
    ComputedPath() = default;
    ~ComputedPath() = default;

    std::vector<glm::vec3> waypoints;
};

class PathfindingModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

public:
    PathfindingModule();
    ~PathfindingModule();

    NON_COPYABLE(PathfindingModule)
    NON_MOVABLE(PathfindingModule)

    int SetNavigationMesh(const std::string& mesh);
    ComputedPath FindPath(glm::vec3 startPos, glm::vec3 endPos);

private:
    struct TriangleInfo
    {
        uint32_t indices[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
        glm::vec3 centre = glm::vec3{0.0f};

        uint32_t adjacentTriangleIndices[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
        uint8_t adjacentTriangleCount = 0;
    };

    std::shared_ptr<Renderer> _renderer;

    std::vector<TriangleInfo> _triangles;
    std::unordered_map<uint32_t, uint32_t[3]> _triangles_to_neighbours;
};