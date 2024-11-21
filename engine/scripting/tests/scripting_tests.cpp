#include <gtest/gtest.h>
#include <log.hpp>
#include <scripting_context.hpp>
#include <sstream>

// Every test will initialize a wren virtual machine, better keep memory requirements low
constexpr ScriptingContext::VMMemoryConfig MEMORY_CONFIG {
    256ull * 4ull, 256ull, 50
};

TEST(ScriptingContextTests, PrintHelloWorld)
{
    ScriptingContext context { MEMORY_CONFIG };

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    auto result = context.InterpretWrenModule("game/tests/hello_world.wren");

    EXPECT_TRUE(result);
    EXPECT_EQ(output.str(), "Hello World!\n");
}

TEST(ScriptingContextTests, ModuleImports)
{
    ScriptingContext context { MEMORY_CONFIG };
    auto result = context.InterpretWrenModule("game/tests/import_modules.wren");
    EXPECT_TRUE(result);
}

TEST(ScriptingContextTests, ImportDirectories)
{
    ScriptingContext context { MEMORY_CONFIG };
    context.AddWrenIncludePath("game/");

    auto result = context.InterpretWrenModule("game/tests/import_same.wren");
    EXPECT_TRUE(result);
    EXPECT_EQ(context.GetModuleCount(), 3);
}