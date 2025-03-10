#include "scripting_module.hpp"
#include "file_io.hpp"
#include "time_module.hpp"
#include "wren_bindings.hpp"

void ScriptingModule::ResetVM(Engine& e)
{
    _mainModule.reset();
    _context->Reset();
    BindEngineAPI(GetForeignAPI());
}

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

    if (!_nextScript.empty())
    {
        ResetVM(engine);
        if (auto result = _context->RunScript(_nextScript))
        {
            _mainModule = std::make_unique<MainScript>(&engine, _context->GetVM(), result.value(), "Main");
            _mainEngineScript = result.value();
        }
        _nextScript.clear();
    }

    if (_mainModule)
    {
        _mainModule->Update(dt);
        _context->FlushOutputStream();
    }
}

void ScriptingModule::SetMainScript(Engine& engine, const std::string& path)
{
    if (_mainEngineScript.empty())
    {
        ResetVM(engine);
        if (auto result = _context->RunScript(path))
        {
            _mainModule = std::make_unique<MainScript>(&engine, _context->GetVM(), result.value(), "Main");
            _mainEngineScript = result.value();
        }
    }
    else
    {
        _nextScript = path;
    }
}

void ScriptingModule::HotReload(Engine& e)
{
    SetMainScript(e, _mainEngineScript);
};
