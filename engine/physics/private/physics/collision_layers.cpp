#include "physics/collision_layers.hpp"

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case eNON_MOVING_OBJECT:
            return inObject2 == eMOVING_OBJECT; // Non moving only collides with moving
        case eMOVING_OBJECT:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

/// BroadPhaseLayerInterface implementation
/// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[eNON_MOVING_OBJECT] = JPH::BroadPhaseLayer { eNON_MOVING_BROADPHASE };
        mObjectToBroadPhase[eMOVING_OBJECT] = JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return eNUM_LAYERS_BROADPHASE;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < eNUM_LAYERS_BROADPHASE);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch (inLayer.GetValue())
        {
        case eNON_MOVING_BROADPHASE:
            return "NON_MOVING";
        case eMOVING_BROADPHASE:
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[eNUM_LAYERS_BROADPHASE];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case eNON_MOVING_OBJECT:
            return inLayer2 == JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
        case eMOVING_OBJECT:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

std::unique_ptr<JPH::ObjectLayerPairFilter> MakeObjectPairFilterImpl()
{
    return std::make_unique<ObjectLayerPairFilterImpl>();
}

std::unique_ptr<JPH::BroadPhaseLayerInterface> MakeBroadPhaseLayerImpl()
{
    return std::make_unique<BPLayerInterfaceImpl>();
}

std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> MakeObjectVsBroadPhaseLayerFilterImpl()
{
    return std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
}