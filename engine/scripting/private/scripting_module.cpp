#include "scripting_module.hpp"
#include "file_io.hpp"
#include "time_module.hpp"
#include "wren_bindings.hpp"

void ScriptingModule::GenerateEngineBindingsFile()
{
    if (auto stream = fileIO::OpenWriteStream(_engineBindingsPath, fileIO::TEXT_WRITE_FLAGS))
    {
        auto& output = stream.value();

        auto& module = GetForeignAPI();
        auto out_script = module.str();

        // Craft the header of the Generated File
        output << "// Automatically generated file: DO NOT MODIFY!\n";
        output << "\n";

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
    _engineBindingsPath = fileIO::CanonicalizePath("game/engine_api.wren");

    return ModuleTickOrder::ePreTick;
}

void ScriptingModule::Tick(Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime();

    if (_mainModule)
    {
        _mainModule->Update(dt);
        _context->FlushOutputStream();
    }
}

void ScriptingModule::SetMainScript(Engine& e, const std::string& path)
{
    _mainModule.reset();
    _context->Reset();
    BindEngineAPI(GetForeignAPI());
    _mainEngineScript = path;
    if (auto result = _context->RunScript(_mainEngineScript))
    {
        _mainModule = std::make_unique<MainScript>(&e, _context->GetVM(), result.value(), "Main");
    }
}

void ScriptingModule::HotReload(Engine& e)
{
    _mainModule.reset();
    _context->Reset();
    BindEngineAPI(GetForeignAPI());

    if (auto result = _context->RunScript(_mainEngineScript))
    {
        _mainModule = std::make_unique<MainScript>(&e, _context->GetVM(), result.value(), "Main");
    }
};
