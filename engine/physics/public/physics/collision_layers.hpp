#pragma once
#include "Jolt/Jolt.h"

// This is probably the simplest setup you can have

enum PhysicsObjectLayer : JPH::ObjectLayer {
    eNON_MOVING_OBJECT,
    eMOVING_OBJECT,
    eNUM_LAYERS_OBJECT
};

enum PhysicsBroadphaseLayer : JPH::BroadPhaseLayer::Type {
    eNON_MOVING_BROADPHASE,
    eMOVING_BROADPHASE,
    eNUM_LAYERS_BROADPHASE
};

std::unique_ptr<JPH::ObjectLayerPairFilter> MakeObjectPairFilterImpl();
std::unique_ptr<JPH::BroadPhaseLayerInterface> MakeBroadPhaseLayerImpl();
std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> MakeObjectVsBroadPhaseLayerFilterImpl();