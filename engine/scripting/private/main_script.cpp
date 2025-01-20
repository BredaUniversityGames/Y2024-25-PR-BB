#include "main_script.hpp"
#include "log.hpp"
#include "wren_engine.hpp"

#include <tracy/Tracy.hpp>

void MainScript::SetMainScript(wren::VM& vm, const std::string& module, const std::string& className)
{
    try
    {
        mainClass = vm.find(module, className);
        mainUpdate = mainClass.func("Update(_,_)");
        mainInit = mainClass.func("Start(_)");

        valid = true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
        valid = false;
    }
}

void MainScript::InitMainScript(Engine* e)
{
    try
    {
        mainInit(WrenEngine { e });
        valid = true;
    }
    catch (wren::Exception& ex)
    {
        bblog::error(ex.what());
        valid = false;
    }
}

void MainScript::Update(Engine* e, DeltaMS deltatime)
{
    try
    {
        mainUpdate(WrenEngine { e }, deltatime.count());
        valid = true;
    }
    catch (wren::Exception& ex)
    {
        bblog::error(ex.what());
        valid = false;
    }
}