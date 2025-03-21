#include <gtest/gtest.h>
#include <sstream>

#include "file_io.hpp"
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
    ScriptingContext context { MEMORY_CONFIG };
    context.GetVM().module("Engine.wren").klass<WrenEngine>("Engine");

    std::stringstream output;
    context.SetScriptingOutputStream(&output);

    auto result = context.RunScript("game/tests/wren_main.wren");
    ASSERT_TRUE(result.has_value());

    MainScript wrenMain { nullptr, context.GetVM(), result.value_or(""), "ExampleMain" };
    wrenMain.Update(DeltaMS { 10.0f }); // Safe, the script does not use the engine parameter

    EXPECT_TRUE(wrenMain.IsValid());
    EXPECT_EQ(output.str(), "10\n");
}