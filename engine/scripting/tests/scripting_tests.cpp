#include "scripting_context.hpp"
#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>

// Every test will initialize a wren virtual machine, better keep memory requirements low
const VMInitConfig MEMORY_CONFIG {
    { "./", "./game/tests/", "./game/" }, 256ull * 4ull, 256ull, 50
};

TEST(ScriptingContextTests, PrintHelloWorld)
{
    ScriptingContext context { MEMORY_CONFIG };

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    auto result = context.RunScript("game/tests/hello_world.wren");

    EXPECT_TRUE(result);
    EXPECT_EQ(oss.str(), "[Script] Hello World!\r\n");
}

TEST(ScriptingContextTests, ModuleImports)
{
    ScriptingContext context { MEMORY_CONFIG };

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    auto result = context.RunScript("game/tests/import_modules.wren");
    EXPECT_TRUE(result);
    EXPECT_EQ(oss.str(), "[Script] Hello World!\r\n");
}
