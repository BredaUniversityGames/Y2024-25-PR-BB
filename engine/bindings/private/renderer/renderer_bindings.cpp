#include "renderer_bindings.hpp"

#include "renderer_module.hpp"

#include "components/static_mesh_component.hpp"

#include <components/skinned_mesh_component.hpp>
#include <graphics_context.hpp>
#include <graphics_resources.hpp>
#include <renderer.hpp>
#include <resource_management/mesh_resource_manager.hpp>

namespace bindings
{
ResourceHandle<GPUMesh> GetMesh(RendererModule& self, std::string name)
{
    return self.GetRenderer()->GetContext()->Resources()->MeshResourceManager().TryGetMesh(name);
}

template <typename T>
void SetMesh(WrenComponent<T>& self, ResourceHandle<GPUMesh> mesh)
{
    self.component->mesh = mesh;
}

}

void BindRendererAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<RendererModule>("Renderer");
    wren_class.funcExt<&bindings::GetMesh>("GetMesh");

    auto& staticMesh = module.klass<WrenComponent<StaticMeshComponent>>("StaticMeshComponent");
    staticMesh.funcExt<&bindings::SetMesh<StaticMeshComponent>>("SetMesh");

    auto& skinnedMesh = module.klass<WrenComponent<SkinnedMeshComponent>>("SkinnedMeshComponent");
    skinnedMesh.funcExt<&bindings::SetMesh<SkinnedMeshComponent>>("SetMesh");

    module.klass<ResourceHandle<GPUMesh>>("GPUMesh");
}
