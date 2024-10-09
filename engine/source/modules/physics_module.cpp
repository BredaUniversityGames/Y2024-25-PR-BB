#include "modules/physics_module.hpp"

#include "../../../build/WSL-Debug/_deps/joltphysics-src/Jolt/Renderer/DebugRendererSimple.h"
PhysicsModule::PhysicsModule()
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
    temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    broad_phase_layer_interface = new BPLayerInterfaceImpl();

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

    // Now we can create the actual physics system.
    physics_system = new JPH::PhysicsSystem();
    physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);
    physics_system->SetGravity(JPH::Vec3Arg(0, 0, 0));

    debug_renderer = new MyDebugRenderer();
    JPH::DebugRenderer::sInstance = debug_renderer;
    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    body_activation_listener
        = new MyBodyActivationListener();
    physics_system->SetBodyActivationListener(body_activation_listener);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    contact_listener = new MyContactListener();
    physics_system->SetContactListener(contact_listener);

    // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
    // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
    body_interface = &physics_system->GetBodyInterface();

    // just for testing now
}
PhysicsModule::~PhysicsModule()
{
    JPH::UnregisterTypes();
    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete temp_allocator;
    delete job_system;
}
void PhysicsModule::UpdatePhysicsEngine(float deltaTime)
{
    // Step the world
    physics_system->Update(1.0 / 60.0, cCollisionSteps, temp_allocator, job_system);
}