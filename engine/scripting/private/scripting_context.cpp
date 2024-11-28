#include "scripting_context.hpp"
#include "file_io.hpp"
#include "log.hpp"
#include "utility/script_file_loading.hpp"

ScriptingContext::ScriptingContext(const VMInitConfig& info)
{
    std::vector<std::string> correctedPaths;
    for (const auto& include_dir : info.includePaths)
    {
        correctedPaths.emplace_back(ScriptFileLoading::CanonicalizePath(include_dir));
    }

    _vm = std::make_unique<wren::VM>(
        correctedPaths,
        info.initialHeapSize,
        info.minHeapSize,
        info.heapGrowthPercent);

    _vm->setPrintFunc([this](const char* message)
        { *this->_wrenOutStream << message; });

    _vm->setLoadFileFunc(ScriptFileLoading::LoadModuleFromFile);
}

ScriptingContext::~ScriptingContext()
{
    _vm.reset();
}

std::optional<std::string> ScriptingContext::InterpretWrenModule(const std::string& path)
{
    auto correctedPath = ScriptFileLoading::CanonicalizePath(path);

    try
    {
        _vm->runFromModule(correctedPath);
        return correctedPath;
    }
    catch (const std::exception& e)
    {
        bblog::error(e.what());
    }

    return std::nullopt;
}
