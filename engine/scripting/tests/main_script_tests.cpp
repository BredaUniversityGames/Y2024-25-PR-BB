#include <gtest/gtest.h>
#include <sstream>

#include "main_script.hpp"
#include "scripting_context.hpp"
#include "time_module.hpp"

// Every test will initialize a wren virtual machine, better keep memory requirements low
const ScriptingContext::VMInitConfig MEMORY_CONFIG {
    { "./", "./game/tests/", "./game/" }, 256ull * 4ull, 256ull, 50
};

TEST(MainScriptTests, MainScript)
{
    ScriptingContext context { MEMORY_CONFIG };

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    auto result = context.InterpretWrenModule("game/tests/wren_main.wren");
    ASSERT_TRUE(result.has_value());

    MainScript wrenMain {};
    wrenMain.SetMainModule(context.GetVM(), *result, "ExampleMain");
    wrenMain.Update(DeltaMS { 10.0f });

    EXPECT_TRUE(wrenMain.IsValid());
    EXPECT_EQ(output.str(), "10\n");
}