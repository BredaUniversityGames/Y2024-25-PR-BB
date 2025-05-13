#include "physics/collision.hpp"

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case eSTATIC:
        {
            return inObject2 != eSTATIC;
        }
        case ePLAYER:
        {
            return inObject2 == eSTATIC || inObject2 == eINTERACTABLE || inObject2 == eENEMY || inObject2 == ePROJECTILE;
        }
        case eINTERACTABLE:
        {
            return inObject2 == eSTATIC || inObject2 == ePLAYER;
        }
        case ePROJECTILE:
        {
            return inObject2 == eSTATIC || inObject2 == ePLAYER || inObject2 == eENEMY;
        }
        case eENEMY:
        {
            return inObject2 == eSTATIC || inObject2 == ePLAYER || inObject2 == ePROJECTILE;
        }
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
        mObjectToBroadPhase[eSTATIC] = JPH::BroadPhaseLayer { eNON_MOVING_BROADPHASE };

        mObjectToBroadPhase[eINTERACTABLE] = JPH::BroadPhaseLayer { eMOVING_BROADPHASE };

        mObjectToBroadPhase[ePROJECTILE] = JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
        mObjectToBroadPhase[ePLAYER] = JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
        mObjectToBroadPhase[eENEMY] = JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return eNUM_BROADPHASE_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < eNUM_OBJECT_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    NO_DISCARD virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
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
    JPH::BroadPhaseLayer mObjectToBroadPhase[eNUM_OBJECT_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case eSTATIC:
            return inLayer2 == JPH::BroadPhaseLayer { eMOVING_BROADPHASE };
        default:
            return true;
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