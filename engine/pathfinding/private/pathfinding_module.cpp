#include "pathfinding_module.h"

#include <engine.hpp>
#include <model_loader.hpp>
#include <renderer_module.hpp>
#include <set>

ModuleTickOrder PathfindingModule::Init(Engine& engine)
{
    _renderer = engine.GetModule<RendererModule>().GetRenderer();

    this->SetNavigationMesh("assets/models/NavMesh.gltf");

    return ModuleTickOrder::eTick;
}

void PathfindingModule::Tick(Engine& engine)
{
    (void)engine;
}

void PathfindingModule::Shutdown(Engine& engine)
{
    (void)engine;
}


PathfindingModule::PathfindingModule()
{

}

PathfindingModule::~PathfindingModule()
{

}

struct TriangleInfo
{
    uint32_t indices[3] = {UINT32_MAX};
    glm::vec3 centre = glm::vec3{0.0f};

    uint32_t adjacentTriangleIndices[3] = {};
    uint8_t adjacentTriangleCount = 0;
};

int PathfindingModule::SetNavigationMesh(const std::string& mesh_path)
{
    CPUModel navmesh = _renderer->GetModelLoader().ExtractModelFromGltfFile(mesh_path);
    CPUMesh navmesh_mesh = navmesh.meshes[0]; // GLTF model should consist of only a single mesh

    std::unordered_map<uint32_t, std::vector<uint32_t>> indices_to_triangles;

    // We store all the triangles with their indices and center points
    //
    // We also store the triangles that use an index to find adjacent triangles
    std::vector<TriangleInfo> triangles;
    for(const auto& primitive : navmesh_mesh.primitives)
    {
        for(size_t i = 0; i < primitive.indices.size(); i += 3)
        {
            TriangleInfo info{};
            info.indices[0] = primitive.indices[i];
            info.indices[1] = primitive.indices[i+1];
            info.indices[2] = primitive.indices[i+2];

            glm::vec3 p0 = primitive.vertices[info.indices[0]].position;
            glm::vec3 p1 = primitive.vertices[info.indices[1]].position;
            glm::vec3 p2 = primitive.vertices[info.indices[2]].position;

            info.centre = (p0 + p1 + p2) / 3.0f;

            size_t triangle_idx = triangles.size();
            triangles.push_back(info);

            indices_to_triangles[info.indices[0]].push_back(triangle_idx);
            indices_to_triangles[info.indices[1]].push_back(triangle_idx);
            indices_to_triangles[info.indices[2]].push_back(triangle_idx);
        }
    }

    // Loop over all triangles in the mesh
    for(size_t i = 0; i < triangles.size(); i++)
    {
        TriangleInfo& triangle = triangles[i];

        // We store all triangles that share indices with the current triangle
        // If a triangle shares two indices with the current triangle, it's adjacent to the current triangle
        std::multiset<uint32_t> shared_triangles;

        // For each index in the current triangle
        for(uint32_t j = 0; j < 3; j++)
        {
            // Get all triangles that use this index
            const std::vector<uint32_t>& index_triangles = indices_to_triangles[triangle.indices[j]];

            // Loop over every triangle that shares this index
            for(uint32_t triangle_idx : index_triangles)
            {
                // Don't add current triangle to list, obviously
                if(triangle_idx == i)
                    continue;

                shared_triangles.insert(triangle_idx);
            }
        }

        // Loop over all triangles that share indices with the current triangle
        for(const uint32_t& triangle_idx : shared_triangles)
        {
            if(shared_triangles.count(triangle_idx) > 1)
            {
                // We share more than 2 indices now so we're adjacent to the current triangle
                triangle.adjacentTriangleIndices[triangle.adjacentTriangleCount++] = triangle_idx;
            }
        }
    }

    // Now that we have adjacency information about the triangles we can start constructing a node tree


    return 0;
}