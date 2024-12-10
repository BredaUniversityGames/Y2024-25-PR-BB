#pragma once
#include "common.hpp"
#include "engine.hpp"
#include "log.hpp"
#include "system_interface.hpp"
#include "utility/entity_serializer.hpp"

#include <cassert>
#include <entt/entity/registry.hpp>

struct DeleteTag
{
};

class ECSModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

    void UpdateSystems(float dt);
    void RenderSystems() const;
    void RemovedDestroyed();

public:
    void DestroyEntity(entt::entity entity);

    ECSModule() = default;
    ~ECSModule() override = default;

    NON_COPYABLE(ECSModule);
    NON_MOVABLE(ECSModule);

    entt::registry& GetRegistry() { return registry; }
    const entt::registry& GetRegistry() const { return registry; }
    std::vector<std::unique_ptr<SystemInterface>>& GetSystems() { return systems; }

    template <typename T, typename... Args>
    void AddSystem(Args&&... args)
        requires IsSystem<T>;

    // Returns nullptr if no System T is found
    template <typename T>
    T* GetSystem()
        requires IsSystem<T>;

    template <typename A>
    void save(A& archive, MAYBE_UNUSED uint32_t version) const;

private:
    entt::registry registry {};
    std::vector<std::unique_ptr<SystemInterface>> systems {};
};

template <typename T, typename... Args>
void ECSModule::AddSystem(Args&&... args)
    requires IsSystem<T>
{
    systems.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    bblog::info("{}, created", typeid(T).name());
}

template <typename T>
T* ECSModule::GetSystem()
    requires IsSystem<T>
{
    for (auto& s : systems)
    {
        T* found = dynamic_cast<T*>(s.get());
        return found;
    }
    assert(false && "Could not find system");
    return nullptr;
}

template <typename A>
void ECSModule::save(A& archive, MAYBE_UNUSED uint32_t version) const
{
    auto entityView = registry.view<entt::entity>();
    for (auto entity : entityView)
    {
        archive(EntitySerializer(registry, entity));
    }
}

CEREAL_CLASS_VERSION(ECSModule, 0);