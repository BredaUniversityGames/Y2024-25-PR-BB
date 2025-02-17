#include "main_script.hpp"
#include "log.hpp"
#include "wren_engine.hpp"

#include <tracy/Tracy.hpp>

MainScript::MainScript(Engine* e, wren::VM& vm, const std::string& module, const std::string& className)
{
    engine = e;
    valid = true;

    try
    {
        mainClass = vm.find(module, className);
        mainUpdate = mainClass.func("Update(_,_)");
        mainInit = mainClass.func("Start(_)");
        mainShutdown = mainClass.func("Shutdown(_)");
    }
    catch (wren::Exception& e)
    {
        bblog::error(e.what());
        valid = false;
        return;
    }

    try
    {
        mainInit(WrenEngine { engine });
    }
    catch (wren::Exception& ex)
    {
        bblog::error(ex.what());
        valid = false;
        return;
    }
}

MainScript::~MainScript()
{
    if (valid == false)
        return;

    try
    {
        mainShutdown(WrenEngine { engine });
    }
    catch (wren::Exception& ex)
    {
        bblog::error(ex.what());
    }
}

void MainScript::Update(DeltaMS deltatime)
{
    if (valid == false)
        return;

    try
    {
        mainUpdate(WrenEngine { engine }, deltatime.count());
    }
    catch (wren::Exception& ex)
    {
        bblog::error(ex.what());
        valid = false;
    }
}