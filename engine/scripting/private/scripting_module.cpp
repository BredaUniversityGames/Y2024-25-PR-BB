#include <scripting_module.hpp>
#include <time_module.hpp>

void ScriptingModule::GenerateEngineBindingsFile(const std::string& path)
{
    if (auto stream = fileIO::OpenWriteStream(path, fileIO::TEXT_WRITE_FLAGS))
    {
        _foreign_bindings.WriteToStream(_context->GetVM(), stream.value());
    }
}

ModuleTickOrder ScriptingModule::Init(MAYBE_UNUSED Engine& engine)
{
    ScriptingContext::VMInitConfig config {};
    config.includePaths.emplace_back("./");
    config.includePaths.emplace_back("./game/");

    _context = std::make_unique<ScriptingContext>(config);
    _mainModule = std::make_unique<MainScript>();

    return ModuleTickOrder::ePreTick;
}

void ScriptingModule::Tick(Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime();
    _mainModule->Update(dt);

    _context->FlushOutputStream();
}

void ScriptingModule::SetMainScript(const std::string& path)
{
    if (auto result = _context->InterpretWrenModule(path))
    {
        _mainModule->SetMainModule(_context->GetVM(), result.value(), "Main");
    }
};
