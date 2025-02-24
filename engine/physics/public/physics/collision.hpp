#pragma once
#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

enum PhysicsObjectLayer : JPH::ObjectLayer
{
    eNON_MOVING_OBJECT,
    eMOVING_OBJECT,
    eNUM_LAYERS_OBJECT
};

enum PhysicsBroadphaseLayer : JPH::BroadPhaseLayer::Type
{
    eNON_MOVING_BROADPHASE,
    eMOVING_BROADPHASE,
    eNUM_LAYERS_BROADPHASE
};

enum class PhysicsShapes
{
    eSPHERE,
    eBOX,
    eCAPSULE,
    eCUSTOM,
    eCONVEXHULL,
    eMESH,
};

enum class BodyType
{
    eDYNAMIC,
    eSTATIC,
};

std::unique_ptr<JPH::ObjectLayerPairFilter> MakeObjectPairFilterImpl();
std::unique_ptr<JPH::BroadPhaseLayerInterface> MakeBroadPhaseLayerImpl();
std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> MakeObjectVsBroadPhaseLayerFilterImpl();