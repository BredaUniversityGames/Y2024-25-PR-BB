#include "main_script.hpp"
#include "log.hpp"

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
        valid = false;
    }
}

void MainScript::Update(DeltaMS deltatime)
{
    try
    {
        mainUpdate(deltatime.count());
        valid = true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
        valid = false;
    }
}