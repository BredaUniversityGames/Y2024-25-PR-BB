#include <glm/vec3.hpp>
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

#include "wren_bindings.hpp"

#include "log.hpp"
#include "scripting_context.hpp"
#include "scripting_module.hpp"
#include "time_module.hpp"

#include "main_engine.hpp"
#include "wren_engine.hpp"

#include <file_io.hpp>

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
    fileIO::Init(true);

    ScriptingContext context { MEMORY_CONFIG };

    auto& vm = context.GetVM();

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

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
    EXPECT_NE(oss.str().find("[Script] 1, 2, 3"), std::string::npos);

    fileIO::Deinit();
}

TEST(ForeignDataTests, EngineWrapper)
{
    fileIO::Init(true);

    MainEngine e {};
    e.AddModule<TimeModule>();

    auto& scripting = e.GetModule<ScriptingModule>();
    scripting.SetEngineBindingsPath("Engine.wren");

    auto& context = scripting.GetContext();

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    // Engine Binding

    BindEngineAPI(scripting.GetForeignAPI());

    auto script = context.RunScript("game/tests/foreign_engine.wren");
    ASSERT_TRUE(script);

    auto test_class = context.GetVM().find(script.value_or(""), "Test");
    test_class.func("test(_)")(WrenEngine { &e });

    EXPECT_NE(oss.str().find("[Script] 0"), std::string::npos);

    fileIO::Deinit();
}