#pragma once
#include "common.hpp"
#include "profile_macros.hpp"

#include <tracy/TracyC.h>

class WrenTracyZone
{
public:
    WrenTracyZone(const std::string& name)
        : _name(name)
    {
        TracyCZone(ctx, true);
        TracyCZoneName(ctx, name.c_str(), name.size());

        _context = ctx;
        _active = true;
    }

    NON_COPYABLE(WrenTracyZone);
    NON_MOVABLE(WrenTracyZone);

    void End()
    {
        TracyCZoneEnd(_context);
        _active = false;
    }

    ~WrenTracyZone()
    {
        if (_active)
            End();
    }

private:
    TracyCZoneCtx _context {};
    std::string _name;
    bool _active = true;
};