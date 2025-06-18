#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

#include "file_io.hpp"
#include "log.hpp"
#include "main_script.hpp"
#include "scripting_context.hpp"
#include "time_module.hpp"
#include "wren_engine.hpp"

// Every test will initialize a wren virtual machine, better keep memory requirements low
const VMInitConfig MEMORY_CONFIG {
    { "", "./game/tests/", "./game/" }, 256ull * 4ull, 256ull, 50
};

TEST(MainScriptTests, MainScript)
{
    fileIO::FileSystem system { true };

    ScriptingContext context { MEMORY_CONFIG };
    context.GetVM().module("Engine.wren").klass<WrenEngine>("Engine");

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    auto result = context.RunScript("game/tests/wren_main.wren");
    ASSERT_TRUE(result.has_value());

    MainScript wrenMain { nullptr, context.GetVM(), result.value_or(""), "ExampleMain" };
    wrenMain.Update(DeltaMS { 10.0f }); // Safe, the script does not use the engine parameter

    EXPECT_TRUE(wrenMain.IsValid());
    EXPECT_NE(oss.str().find("[Script] 10"), std::string::npos);
}