#pragma once
#include "log.hpp"
#include "wren_common.hpp"

#include <variant>

struct Callback
{
    Callback() = default;

    Callback(std::function<void()> func)
    {
        fn = func;
    }

    Callback(wren::Variable wrenLambda)
    {
        try
        {
            fn = wrenLambda.func("call()");
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
        void operator()(std::function<void()>& fn) const { fn(); };

        void operator()(wren::Method& fn) const
        {
            try
            {
                fn();
            }
            catch (wren::Exception& ex)
            {
                bblog::error(ex.what());
            }
        };
    };

public:
    void operator()()
    {
        std::visit(CallVisitor {}, fn);
    }

private:
    using Fn = std::variant<std::monostate, std::function<void()>, wren::Method>;
    Fn fn {};
};