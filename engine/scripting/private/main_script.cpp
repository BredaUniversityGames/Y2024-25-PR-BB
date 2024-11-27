#include "main_script.hpp"

void MainScript::SetMainModule(wren::VM& vm, const std::string& module, const std::string& className)
{
    try
    {
        mainClass = vm.find(module, className);
        mainUpdate = mainClass.func("Update(_)");
        mainClass.func("Start()")();
        valid = true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
    }
}

bool MainScript::Update(DeltaMS deltatime)
{
    if (!valid)
        return true;

    try
    {
        mainUpdate(deltatime.count());
        return true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
    }

    return false;
}