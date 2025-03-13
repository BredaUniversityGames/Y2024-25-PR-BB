#include "renderer_bindings.hpp"

#include "components/skinned_mesh_component.hpp"
#include "components/static_mesh_component.hpp"
#include "entity/wren_entity.hpp"

#include "cpu_resources.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "renderer_module.hpp"
#include "resource_management/model_resource_manager.hpp"

namespace bindings
{
ModelResourceManager& GetModelResourceManager(RendererModule& self)
{
    return self.GetGraphicsContext()->Resources()->ModelResourceManager();
}

static const ResourceHandle<GPUMesh>& GetStaticMeshGPUMeshRef(WrenComponent<StaticMeshComponent>& self)
{
    return self.component->mesh;
}

static void SetStaticMeshGPUMeshRef(WrenComponent<StaticMeshComponent>& self, const ResourceHandle<GPUMesh>& mesh)
{
    self.component->mesh = mesh;
}

static const ResourceHandle<GPUMesh>& GetSkinnedMeshGPUMeshRef(WrenComponent<SkinnedMeshComponent>& self)
{
    return self.component->mesh;
}

static void SetSkinnedMeshGPUMeshRef(WrenComponent<SkinnedMeshComponent>& self, const ResourceHandle<GPUMesh>& mesh)
{
    self.component->mesh = mesh;
}

ResourceHandle<GPUModel> LoadCPUModelIntoRenderer(RendererModule& self, CPUModel& model)
{
    return self.LoadModels({ model })[0];
}
const GPUModel& AccessGPUModel(ModelResourceManager& self, ResourceHandle<GPUModel>& modelHandle)
{
    return *self.Access(modelHandle);
}
std::vector<ResourceHandle<GPUMesh>>& GetStaticMeshes(GPUModel& self)
{
    return self.staticMeshes;
}
std::vector<ResourceHandle<GPUMesh>>& GetSkinnedMeshes(GPUModel& self)
{
    return self.skinnedMeshes;
}
}

void BindRendererAPI(wren::ForeignModule& module)
{
    // Component registration
    auto& staticMeshComponent = module.klass<WrenComponent<StaticMeshComponent>>("StaticMeshComponent");
    staticMeshComponent.propExt<bindings::GetStaticMeshGPUMeshRef, bindings::SetStaticMeshGPUMeshRef>("mesh");
    auto& skinnedMeshComponent = module.klass<WrenComponent<SkinnedMeshComponent>>("SkinnedMeshComponent");
    skinnedMeshComponent.propExt<bindings::GetSkinnedMeshGPUMeshRef, bindings::SetSkinnedMeshGPUMeshRef>("mesh");

    // object registrations
    MAYBE_UNUSED auto& cpuModelClass = module.klass<CPUModel>("CPUModel");
    MAYBE_UNUSED auto& gpuModelClass = module.klass<GPUModel>("GPUModel");
    MAYBE_UNUSED auto& gpuModelHandleClass = module.klass<ResourceHandle<GPUModel>>("GPUModelHandle");
    MAYBE_UNUSED auto& gpuMeshHandleClass = module.klass<ResourceHandle<GPUMesh>>("GPUMeshHandle");
    MAYBE_UNUSED auto& modelResourceManagerClass = module.klass<ModelResourceManager>("ModelResourceManager");
    MAYBE_UNUSED auto& rendererClass = module.klass<RendererModule>("RendererModule");

    // Function binding registration
    rendererClass.funcExt<bindings::LoadCPUModelIntoRenderer>("LoadCPUModelIntoRenderer");
    rendererClass.funcExt<bindings::GetModelResourceManager>("GetModelResourceManager");
    modelResourceManagerClass.funcExt<bindings::AccessGPUModel>("Access");
    gpuModelClass.propReadonlyExt<bindings::GetStaticMeshes>("staticMeshes");
    gpuModelClass.propReadonlyExt<bindings::GetSkinnedMeshes>("skinnedMeshes");
}