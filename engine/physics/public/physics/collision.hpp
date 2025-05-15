#pragma once

#include "common.hpp"
#include <memory>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

enum PhysicsObjectLayer : JPH::ObjectLayer
{
    eSTATIC, // For walls, floor, map props
    ePLAYER, // Player collider
    eENEMY, // For enemy hit colliders
    ePROJECTILE, // For projectiles or explosions (both enemies and player created)
    eINTERACTABLE, // For pickups, doors and interactables
    eCOINS, // For coins/gold
    eNUM_OBJECT_LAYERS
};

enum PhysicsBroadphaseLayer : JPH::BroadPhaseLayer::Type
{
    eNON_MOVING_BROADPHASE,
    eMOVING_BROADPHASE,
    eNUM_BROADPHASE_LAYERS
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

std::unique_ptr<JPH::ObjectLayerPairFilter> MakeObjectPairFilterImpl();
std::unique_ptr<JPH::BroadPhaseLayerInterface> MakeBroadPhaseLayerImpl();
std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> MakeObjectVsBroadPhaseLayerFilterImpl();