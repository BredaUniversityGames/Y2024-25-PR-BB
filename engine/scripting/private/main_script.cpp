#include "main_script.hpp"

void MainScript::SetMainModule(wren::VM& vm, const std::string& module, const std::string& className)
{
    mainClass = vm.find(module, className);
    mainUpdate = mainClass.func("update(_)");
}

bool MainScript::Update(DeltaMS deltatime)
{
    try
    {
        mainUpdate(deltatime.count());
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
    }
}