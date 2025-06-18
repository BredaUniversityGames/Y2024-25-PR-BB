#include "scripting_context.hpp"

#include <file_io.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <physfs.hpp>
#include <spdlog/sinks/ostream_sink.h>

// Every test will initialize a wren virtual machine, better keep memory requirements low
const VMInitConfig MEMORY_CONFIG {
    { "./", "./game/tests/", "./game/" }, 256ull * 4ull, 256ull, 50
};

TEST(ScriptingContextTests, PrintHelloWorld)
{
    fileIO::FileSystem system { true };

    ScriptingContext context { MEMORY_CONFIG };

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    auto result = context.RunScript("game/tests/hello_world.wren");

    EXPECT_TRUE(result);
    EXPECT_NE(oss.str().find("[Script] Hello World!"), std::string::npos);
}

TEST(ScriptingContextTests, ModuleImports)
{
    fileIO::FileSystem system { true };

    ScriptingContext context { MEMORY_CONFIG };

    std::ostringstream oss;
    auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test", ostream_sink);
    test_logger->set_pattern("%v");

    context.SetScriptingOutputStream(test_logger);

    auto result = context.RunScript("game/tests/import_modules.wren");
    EXPECT_TRUE(result);
    EXPECT_NE(oss.str().find("[Script] Hello World!"), std::string::npos);
}
