#include "scripting_module.hpp"
#include "file_io.hpp"
#include "time_module.hpp"

void ScriptingModule::GenerateEngineBindingsFile()
{
    if (auto stream = fileIO::OpenWriteStream(_engineBindingsPath, fileIO::TEXT_WRITE_FLAGS))
    {
        auto& output = stream.value();

        auto& module = GetForeignAPI();
        auto out_script = module.str();

        // Craft the header of the Generated File
        output << "// Automatically generated file: DO NOT MODIFY!\n";
        output << "// This script is purely for documentation\n";

        // Dump module contents
        output << out_script << std::endl;
    }
}

ModuleTickOrder ScriptingModule::Init(MAYBE_UNUSED Engine& engine)
{
    VMInitConfig config {};
    config.includePaths.emplace_back("./");
    config.includePaths.emplace_back("./game/");

    _context = std::make_unique<ScriptingContext>(config);
    _mainModule = std::make_unique<MainScript>();

    _engineBindingsPath = fileIO::CanonicalizePath("game/engine_api.wren");

    return ModuleTickOrder::ePreTick;
}

void ScriptingModule::Tick(Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime();

    if (_mainModule->IsValid())
    {
        _mainModule->Update(&engine, dt);
        _context->FlushOutputStream();
    }
}

void ScriptingModule::SetMainScript(Engine& e, const std::string& path)
{
    if (auto result = _context->RunScript(path))
    {
        _mainModule->SetMainScript(_context->GetVM(), result.value(), "Main");
        _mainModule->InitMainScript(&e);
    }
};
