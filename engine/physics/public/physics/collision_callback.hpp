#pragma once
#include "log.hpp"
#include "wren_common.hpp"
#include "wren_entity.hpp"

#include <functional>
#include <variant>

struct CollisionCallback
{
    CollisionCallback() = default;

    CollisionCallback(std::function<void(WrenEntity, WrenEntity)> func)
    {
        fn = func;
    }

    CollisionCallback(wren::Variable wrenLambda)
    {
        try
        {
            fn = wrenLambda.func("call(_,_)");
        }
        catch (const wren::Exception& e)
        {
            bblog::warn("[WREN WARNING] could not bind wren lambda to callback: {}", e.what());
        }
    }

private:
    struct CallVisitor
    {
        void operator()(std::monostate) const { };
        void operator()(std::function<void(WrenEntity, WrenEntity)>& fn) const { fn(a, b); };

        void operator()(wren::Method& fn) const
        {
            try
            {
                fn(a, b);
            }
            catch (wren::Exception& ex)
            {
                bblog::error(ex.what());
            }
        };

        WrenEntity a {};
        WrenEntity b {};
    };

public:
    void operator()(WrenEntity a, WrenEntity b)
    {
        std::visit(CallVisitor { a, b }, fn);
    }

private:
    using Fn = std::variant<std::monostate, std::function<void(WrenEntity, WrenEntity)>, wren::Method>;
    Fn fn {};
};