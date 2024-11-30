#include "main_script.hpp"
#include "log.hpp"
#include "utility/engine_wrapper.hpp"

void MainScript::SetMainModule(wren::VM& vm, const std::string& module, const std::string& className)
{
    try
    {
        mainClass = vm.find(module, className);
        mainUpdate = mainClass.func("Update(_)");
        mainInit = mainClass.func("Start()");
        valid = true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
        valid = false;
    }
}
void MainScript::InitMainModule(Engine& e)
{
    try
    {
        mainInit(EngineWrapper { e });
        valid = true;
    }
    catch (wren::Exception& e)
    {
        bblog::error(e.what());
        valid = false;
    }
}

void MainScript::Update(Engine& e, DeltaMS deltatime)
{
    try
    {
        mainUpdate(EngineWrapper { e }, deltatime.count());
        valid = true;
    }
    catch (wren::NotFound& e)
    {
        bblog::error(e.what());
        valid = false;
    }
}