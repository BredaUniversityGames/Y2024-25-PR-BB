#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <sstream>

#include "wren_bindings.hpp"

#include "log.hpp"
#include "scripting_context.hpp"
#include "scripting_module.hpp"
#include "time_module.hpp"

#include "main_engine.hpp"
#include "wren_engine.hpp"

// Every test will initialize a wren virtual machine, better keep memory requirements low
const VMInitConfig MEMORY_CONFIG {
    { "", "./game/tests/", "./game/" },
    256ull * 4ull, 256ull, 50
};

namespace bind
{

glm::vec3 Vec3Identity()
{
    return glm::vec3 {};
}

std::string Vec3ToString(glm::vec3& v)
{
    return fmt::format("{}, {}, {}", v.x, v.y, v.z);
}

}

TEST(ForeignDataTests, ForeignBasicClass)
{
    ScriptingContext context { MEMORY_CONFIG };

    auto& vm = context.GetVM();

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    auto& foreignAPI = vm.module("Engine");
    auto& v3 = foreignAPI.klass<glm::vec3>("Vec3");

    v3.ctor<float, float, float>();
    v3.funcStatic<bind::Vec3Identity>("Identity");
    v3.var<&glm::vec3::x>("x");
    v3.var<&glm::vec3::y>("y");
    v3.var<&glm::vec3::z>("z");
    v3.funcExt<bind::Vec3ToString>("ToString");

    auto result = context.RunScript("game/tests/foreign_data.wren");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(output.str(), "1, 2, 3\n");
}

TEST(ForeignDataTests, EngineWrapper)
{
    MainEngine e {};
    e.AddModule<TimeModule>();

    auto& scripting = e.GetModule<ScriptingModule>();
    scripting.SetEngineBindingsPath("Engine.wren");

    auto& context = scripting.GetContext();

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    // Engine Binding

    BindEngineAPI(scripting.GetForeignAPI());

    auto script = context.RunScript("game/tests/foreign_engine.wren");
    ASSERT_TRUE(script);

    auto test_class = context.GetVM().find(script.value_or(""), "Test");
    test_class.func("test(_)")(WrenEngine { &e });

    EXPECT_EQ(output.str(), "0\n");
}