﻿#include "physics_module.hpp"
#include "ecs_module.hpp"
#include "systems/physics_system.hpp"

ModuleTickOrder PhysicsModule::Init(MAYBE_UNUSED Engine& engine)
{

    // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
    // This needs to be done before any other Jolt function is called.
    JPH::RegisterDefaultAllocator();

    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    // It is not directly used in this example but still required.
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    _tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    _jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    _broadPhaseLayerInterface = new BPLayerInterfaceImpl();

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    _objectVsBroadphaseLayerFilter = new ObjectVsBroadPhaseLayerFilterImpl();

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    _objectVsObjectLayerFilter = new ObjectLayerPairFilterImpl();

    // Now we can create the actual physics system.
    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(_maxBodies, _numBodyMutexes, _maxBodyPairs, _maxContactConstraints, *_broadPhaseLayerInterface, *_objectVsBroadphaseLayerFilter, *_objectVsObjectLayerFilter);
    physicsSystem->SetGravity(JPH::Vec3Arg(0, -9.81, 0));

    debugRenderer = new DebugRendererSimpleImpl();
    JPH::DebugRenderer::sInstance = debugRenderer;
    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    _bodyActivationListener
        = new MyBodyActivationListener();
    physicsSystem->SetBodyActivationListener(_bodyActivationListener);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    _contactListener = new MyContactListener();
    physicsSystem->SetContactListener(_contactListener);

    // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
    // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
    bodyInterface = &physicsSystem->GetBodyInterface();
    // just for testing now

    auto& ecs = engine.GetModule<ECSModule>();
    ecs.AddSystem<PhysicsSystem>(engine, ecs, *this);

    return ModuleTickOrder::ePostTick;
}

void PhysicsModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    JPH::UnregisterTypes();
    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete _tempAllocator;
    delete _jobSystem;
}

void PhysicsModule::Tick(MAYBE_UNUSED Engine& engine)
{
    // Step the world
    // TODO: is this correct? We are ignoring deltatime?
    physicsSystem->Update(1.0 / 60.0, _collisionSteps, _tempAllocator, _jobSystem);

    engine.GetModule<ECSModule>().GetSystem<PhysicsSystem>()->CleanUp();
}

RayHitInfo PhysicsModule::ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const
{
    RayHitInfo hitInfo;

    const JPH::Vec3 start(origin.x, origin.y, origin.z);
    JPH::Vec3 dir(direction.x, direction.y, direction.z);
    dir = dir.Normalized();
    const JPH::RayCast ray(start, dir * distance);
    debugRenderer->AddPersistentLine(ray.mOrigin, ray.mOrigin + ray.mDirection, JPH::Color::sRed);

    JPH::AllHitCollisionCollector<JPH::RayCastBodyCollector> collector;
    physicsSystem->GetBroadPhaseQuery().CastRay(ray, collector);
    const int numHits = static_cast<int>(collector.mHits.size());
    if (numHits < 1)
    {
        return hitInfo;
    }

    const auto firstHit = collector.mHits[0];
    const entt::entity hitEntity = static_cast<entt::entity>(bodyInterface->GetUserData(firstHit.mBodyID));
    const glm::vec3 hitPosition = origin + firstHit.mFraction * ((direction * distance));

    hitInfo.entity = hitEntity;
    hitInfo.position = hitPosition;
    hitInfo.hitFraction = firstHit.mFraction;
    hitInfo.hasHit = true;
    return hitInfo;
}