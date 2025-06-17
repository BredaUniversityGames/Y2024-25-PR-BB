#include "physics_module.hpp"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/RegisterTypes.h>

#include "physics/collision.hpp"
#include "physics/constants.hpp"
#include "physics/contact_listener.hpp"
#include "physics/debug_renderer.hpp"

#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "passes/debug_pass.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "systems/physics_system.hpp"
#include "time_module.hpp"

#include <glm/gtx/rotate_vector.hpp>

PhysicsModule::PhysicsModule() { }
PhysicsModule::~PhysicsModule() { }

ModuleTickOrder PhysicsModule::Init(MAYBE_UNUSED Engine& engine)
{
    // Register allocators for Jolt, uses malloc and free by default
    JPH::RegisterDefaultAllocator();

    // Create a factory, used for making objects out of serialization data
    JPH::Factory::sInstance = new JPH::Factory();

    _debugRenderer = std::make_unique<PhysicsDebugRenderer>();
    JPH::DebugRenderer::sInstance = _debugRenderer.get();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class
    JPH::RegisterTypes();

    _tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(PHYSICS_TEMP_ALLOCATOR_SIZE);

    _jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

    _broadphaseLayerInterface = MakeBroadPhaseLayerImpl();
    _objectVsBroadphaseLayerFilter = MakeObjectVsBroadPhaseLayerFilterImpl();
    _objectVsObjectLayerFilter = MakeObjectPairFilterImpl();

    // Now we can create the actual physics system.
    _physicsSystem = std::make_unique<JPH::PhysicsSystem>();

    _physicsSystem->Init(
        PHYSICS_MAX_BODIES, PHYSICS_MUTEX_COUNT,
        PHYSICS_MAX_BODY_PAIRS, PHYSICS_MAX_CONTACT_CONSTRAINTS,
        *_broadphaseLayerInterface, *_objectVsBroadphaseLayerFilter, *_objectVsObjectLayerFilter);

    _physicsSystem->SetGravity(JPH::Vec3(0, -PHYSICS_GRAVITATIONAL_CONSTANT, 0));

    auto& ecs = engine.GetModule<ECSModule>();
    ecs.AddSystem<PhysicsSystem>(engine, ecs, *this);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    _contactListener = std::make_unique<PhysicsContactListener>(ecs.GetRegistry());
    _physicsSystem->SetContactListener(_contactListener.get());

    RigidbodyComponent::SetupRegistryCallbacks(engine.GetModule<ECSModule>().GetRegistry());

    return ModuleTickOrder::ePreTick;
}

void PhysicsModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    RigidbodyComponent::DisconnectRegistryCallbacks(engine.GetModule<ECSModule>().GetRegistry());
    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsModule::Tick(MAYBE_UNUSED Engine& engine)
{
    float deltatimeSeconds = glm::min(engine.GetModule<TimeModule>().GetDeltatime().count(), PHYSICS_MAX_DT) * 0.001f;

    // This is being optimistic: we always do one collision step no matter how small the dt
    const int updatesNeeded = std::min(static_cast<int>(glm::ceil(deltatimeSeconds / PHYSICS_STEPS_PER_SECOND)), PHYSICS_MAX_STEPS_PER_FRAME);

    // Step the world

    if (updatesNeeded > 0)
    {
        auto error = _physicsSystem->Update(deltatimeSeconds, updatesNeeded, _tempAllocator.get(), _jobSystem.get());

        if (error != JPH::EPhysicsUpdateError::None)
        {
            bblog::error("[PHYSICS] Simulation step error has occurred");
        }
    }

    if (!_debugLayersToRender.empty())
    {
        LayerBodyDrawFilter drawFilter {};
        drawFilter.layersToDraw = _debugLayersToRender;

        JPH::BodyManager::DrawSettings drawSettings;
        _physicsSystem->DrawBodies(drawSettings, _debugRenderer.get(), &drawFilter);
        _debugRenderer->NextFrame();
    }

    if (!_debugLayersToRender.empty() || _drawRays)
    {
        auto& debugDrawer = engine.GetModule<RendererModule>().GetRenderer()->GetDebugPipeline();
        debugDrawer.AddLines(_debugRenderer->GetLinesData());
        debugDrawer.AddLines(_debugRenderer->GetPersistentLinesData());
        _debugRenderer->ClearLines();
    }
}

std::vector<RayHitInfo> PhysicsModule::ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const
{
    std::vector<RayHitInfo> hitInfos;

    const JPH::Vec3 start(origin.x, origin.y, origin.z);
    JPH::Vec3 dir(direction.x, direction.y, direction.z);
    dir = dir.Normalized();
    const JPH::RayCast ray(start, dir * distance);

    if (_drawRays)
    {
        if (_clearDrawnRaysPerFrame)
            _debugRenderer->DrawLine(ray.mOrigin, ray.mOrigin + ray.mDirection, JPH::Color::sRed);
        else
            _debugRenderer->AddPersistentLine(ray.mOrigin, ray.mOrigin + ray.mDirection, JPH::Color::sRed);
    }

    // JPH::AllHitCollisionCollector<JPH::RayCastBodyCollector> collector;
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector2;

    // physicsSystem->GetBroadPhaseQuery().CastRay(ray, collector);

    JPH::RayCastSettings settings;
    _physicsSystem->GetNarrowPhaseQuery().CastRay(JPH::RRayCast(ray), settings, collector2);

    collector2.Sort();

    hitInfos.resize(collector2.mHits.size());
    int32_t iterator = 0;

    for (auto hit : collector2.mHits)
    {

        const entt::entity hitEntity = static_cast<entt::entity>(GetBodyInterface().GetUserData(hit.mBodyID));

        if (hitEntity != entt::null)
        {
            hitInfos[iterator].bodyID = hit.mBodyID;
            hitInfos[iterator].entity = hitEntity;
        }

        hitInfos[iterator].position = origin + hit.mFraction * ((direction * distance));
        hitInfos[iterator].hitFraction = hit.mFraction;

        JPH::BodyLockRead bodyLock(_physicsSystem->GetBodyLockInterface(), hit.mBodyID);

        if (bodyLock.Succeeded())
        {
            const JPH::Body& body = bodyLock.GetBody();
            const auto joltNormal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, ray.GetPointOnRay(hit.mFraction));
            hitInfos[iterator].normal = glm::vec3(joltNormal.GetX(), joltNormal.GetY(), joltNormal.GetZ());
        }
        iterator++;
    }

    return hitInfos;
}

std::vector<RayHitInfo> PhysicsModule::ShootMultipleRays(const glm::vec3& origin, const glm::vec3& direction, float distance, uint32_t numRays, float angle) const
{
    std::vector<RayHitInfo> results;

    if (numRays == 1)
    {
        // Single ray shot straight forward
        auto rayHit = ShootRay(origin, direction, distance);
        if (!rayHit.empty())
        {
            results.insert(results.end(), rayHit.begin(), rayHit.end());
        }
        return results;
    }

    // Calculate the angle step based on the number of rays (ensuring symmetrical distribution)
    float angleStep = glm::radians(angle) / (numRays / 2);

    for (uint32_t i = 0; i < numRays; ++i)
    {
        float angleOffset = (i - (numRays - 1) / 2.0f) * angleStep;
        glm::vec3 rotatedDirection = glm::rotateY(direction, angleOffset);

        auto rayHit = ShootRay(origin, rotatedDirection, distance);

        if (!rayHit.empty())
        {
            results.insert(results.end(), rayHit.begin(), rayHit.end());
        }
    }

    std::sort(results.begin(), results.end(), [&origin](const RayHitInfo& a, const RayHitInfo& b)
        { return glm::distance(origin, a.position) < glm::distance(origin, b.position); });

    return results;
}

void PhysicsModule::SetDebugCameraPosition(const glm::vec3& cameraPos) const
{
    _debugRenderer->SetCameraPos(ToJoltVec3(cameraPos));
}

void PhysicsModule::ResetPersistentDebugLines()
{
    _debugRenderer->ClearPersistentLines();
}