#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <sstream>

#include "log.hpp"
#include "scripting_context.hpp"
#include "scripting_module.hpp"
#include "time_module.hpp"

#include "main_engine.hpp"

// Every test will initialize a wren virtual machine, better keep memory requirements low
const ScriptingContext::VMInitConfig MEMORY_CONFIG {
    { "./", "./game/tests/", "./game/" },
    256ull * 4ull, 256ull, 50
};

TEST(ForeignDataTests, ForeignBasicClass)
{
    ScriptingContext context { MEMORY_CONFIG };
    auto& vm = context.GetVM();

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    auto& module = vm.module("Engine");
    auto& v3 = module.klass<glm::vec3>("Vec3");

    constexpr auto identity = []()
    {
        return glm::vec3 {};
    };

    constexpr auto to_string = [](glm::vec3& v)
    {
        return fmt::format("{}, {}, {}", v.x, v.y, v.z);
    };

    v3.ctor<float, float, float>();
    v3.funcStatic<+identity>("Identity");
    v3.var<&glm::vec3::x>("x");
    v3.var<&glm::vec3::y>("y");
    v3.var<&glm::vec3::z>("z");
    v3.funcExt<+to_string>("ToString");

    auto result = context.InterpretWrenModule("game/tests/foreign_data.wren");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(output.str(), "1, 2, 3\n");
}

TEST(ForeignDataTests, EngineWrapper)
{
    MainEngine e {};
    auto& scripting = e.GetModule<ScriptingModule>();
    e.AddModule<TimeModule>();

    auto& context = scripting.GetContext();

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    // Engine Binding
    {
        auto& engineAPI = scripting.StartEngineBind();
        scripting.BindModule<TimeModule>(engineAPI, "Time");
        scripting.EndEngineBind(e);
    }

    EXPECT_TRUE(context.InterpretWrenModule("tests/foreign_engine.wren"));
    EXPECT_EQ(output.str(), "0\n");
}