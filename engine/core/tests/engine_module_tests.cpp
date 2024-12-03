#include "main_engine.hpp"
#include <gtest/gtest.h>

namespace TestModules
{

class TestModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eFirst;
    };

    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
};

class DependentModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        engine.GetModule<TestModule>();
        return ModuleTickOrder::eFirst;
    };

    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
};

class CheckUpdateModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eTick;
    };

    void Tick(MAYBE_UNUSED Engine& engine) override
    {
        _has_updated = true;
    };

    void Shutdown(MAYBE_UNUSED Engine& engine) override {

    };

public:
    bool _has_updated = false;
};

class SelfDestructModuleFirst : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eFirst;
    };

    void Tick(Engine& engine) override
    {
        engine.SetExit(-1);
    };

    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
};

class SelfDestructModuleLast : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eLast;
    };

    void Tick(Engine& engine) override
    {
        engine.SetExit(-2);
    };

    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
};

class SetAtFreeModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eFirst;
    }

    void Tick(MAYBE_UNUSED Engine& engine) override {};

    void Shutdown(MAYBE_UNUSED Engine& engine) override
    {
        *target = 1;
    };

public:
    uint32_t* target = nullptr;
};

class SetAtFreeModule2 : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eFirst;
    }

    void Tick(MAYBE_UNUSED Engine& engine) override {};

    void Shutdown(MAYBE_UNUSED Engine& engine) override
    {
        *target = 2;
    };

public:
    uint32_t* target = nullptr;
};

};

TEST(EngineModuleTests, ModuleGetter)
{
    MainEngine e {};
    e.AddModule<TestModules::TestModule>();

    EXPECT_NE(e.GetModuleSafe<TestModules::TestModule>(), nullptr)
        << "Test Module was not added successfully";
}

TEST(EngineModuleTests, ModuleDependency)
{
    MainEngine e {};
    e.AddModule<TestModules::DependentModule>();

    EXPECT_NE(e.GetModuleSafe<TestModules::TestModule>(), nullptr)
        << "Test Module was not added, even if it was a dependency of DependantModule";
}

TEST(EngineModuleTests, EngineExit)
{
    MainEngine e {};
    e.AddModule<TestModules::SelfDestructModuleFirst>();

    e.MainLoopOnce();

    auto exitCode = e.GetExitCode();
    EXPECT_EQ(exitCode, -1) << "SelfDestructFirst was not able to set the exit code to -1";
}

TEST(EngineModuleTests, EngineReset)
{
    MainEngine e {};
    e.AddModule<TestModules::SelfDestructModuleFirst>();
    e.MainLoopOnce();

    EXPECT_EQ(e.GetExitCode(), -1);

    e.Reset();
    e.AddModule<TestModules::SelfDestructModuleLast>();
    e.MainLoopOnce();

    EXPECT_EQ(e.GetExitCode(), -2);
}

TEST(EngineModuleTests, ModuleDeallocation)
{
    uint32_t target {};
    {
        MainEngine e {};

        e.GetModule<TestModules::SetAtFreeModule>().target = &target;
        e.GetModule<TestModules::SetAtFreeModule2>().target = &target;
        e.AddModule<TestModules::SelfDestructModuleFirst>();

        e.Reset();

        EXPECT_EQ(target, 1) << "SetAtFreeModule was not deleted last";
    }

    {
        MainEngine e {};
        e.GetModule<TestModules::SetAtFreeModule2>().target = &target;
        e.GetModule<TestModules::SetAtFreeModule>().target = &target;
        e.AddModule<TestModules::SelfDestructModuleFirst>();

        e.Reset();

        EXPECT_EQ(target, 2) << "SetAtFreeModule2 was not deleted last";
    }
}

TEST(EngineModuleTests, ModuleTick)
{
    {
        MainEngine e {};

        e.AddModule<TestModules::CheckUpdateModule>();
        e.AddModule<TestModules::SelfDestructModuleFirst>();

        e.MainLoopOnce();

        EXPECT_EQ(e.GetModule<TestModules::CheckUpdateModule>()._has_updated, false)
            << "CheckUpdateModule should not have ticked, SelfDestructFirst terminates before";
    }
    {
        MainEngine e {};

        e.AddModule<TestModules::CheckUpdateModule>();
        e.AddModule<TestModules::SelfDestructModuleLast>();

        e.MainLoopOnce();

        EXPECT_EQ(e.GetModule<TestModules::CheckUpdateModule>()._has_updated, true)
            << "CheckUpdateModule should have ticked, SelfDestructLast terminates afterwards";
    }
}