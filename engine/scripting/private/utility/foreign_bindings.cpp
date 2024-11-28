#include "utility/foreign_bindings.hpp"
#include "log.hpp"
#include "utility/script_file_loading.hpp"

ForeignBindings::ForeignBindings(const std::string& engineModule)
{
    _engineModule = ScriptFileLoading::CanonicalizePath(engineModule);
}

void ForeignBindings::WriteToStream(wren::VM& vm, std::ostream& output) const
{
    auto& module = GetForeignModule(vm);
    auto out_script = module.str();

    // Craft the header of the Generated File
    output << "// Automatically generated file: DO NOT MODIFY!\n";
    output << "// This script is purely for documentation and should not be imported by path\n";
    output << fmt::format("// To use the bindings, import from {:?}\n\n", _engineModule);

    // Dump module contents
    output << out_script << std::endl;
}

void WrenEngine::SetEngine(const WrenEngine& v)
{
    instance = v.instance;
}

wren::ForeignKlassImpl<WrenEngine>& WrenEngine::BindEngineStart(wren::ForeignModule& api)
{
    auto& engineClass = api.klass<WrenEngine>("EngineClass");

    engineClass.ctor<>();
    engineClass.func<&WrenEngine::SetEngine>("SetEngine");

    return engineClass;
}

void WrenEngine::BindEngineEnd(wren::ForeignModule& api, wren::VM& vm, Engine& e)
{
    api.append("var Engine = EngineClass.new()");

    auto code = fmt::format("import {:?} for Engine, EngineClass", api.getName());
    vm.runFromSource("__EngineInit", code);

    auto globalEngine = vm.find(api.getName(), "Engine");
    globalEngine.func("SetEngine(_)")(WrenEngine { &e });
}
