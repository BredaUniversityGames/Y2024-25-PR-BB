#include "scripting_context.hpp"
#include "file_io.hpp"
#include "log.hpp"
#include "profile_macros.hpp"

#include <filesystem>

namespace detail
{
std::string ResolveImport(
    const std::vector<std::string>& paths,
    const std::string& importer,
    const std::string& name)
{
    using Filepath = std::filesystem::path;

    auto parent = Filepath(importer).parent_path();
    auto relative = (Filepath(parent) / Filepath(name)).lexically_normal().generic_string();

    if (fileIO::Exists(relative))
    {
        return relative;
    }

    for (const auto& path : paths)
    {
        auto composed = (Filepath(path) / Filepath(name)).lexically_normal().generic_string();
        if (fileIO::Exists(composed))
        {
            return composed;
        }
    }

    return name;
}

std::string LoadFile(const std::string& path)
{
    if (auto stream = fileIO::OpenReadStream(path))
    {
        return fileIO::DumpStreamIntoString(stream.value());
    }
    throw wren::NotFound();
}

void* ReallocFn(void* prev, size_t size, MAYBE_UNUSED void* user)
{
    TracyFree(prev);

    if (size == 0)
    {
        std::free(prev);
        return nullptr;
    }

    auto* result = std::realloc(prev, size);
    TracyAlloc(result, size);

    return result;
}

}

ScriptingContext::ScriptingContext(const VMInitConfig& info)
{
    _vmInitConfig = info;

    Reset();
}

ScriptingContext::~ScriptingContext()
{
    _vm.reset();
}

void ScriptingContext::Reset()
{
    _vm = std::make_unique<wren::VM>(
        _vmInitConfig.includePaths,
        _vmInitConfig.initialHeapSize,
        _vmInitConfig.minHeapSize,
        _vmInitConfig.heapGrowthPercent,
        detail::ReallocFn);

    auto logHandler = [this](const char* message)
    {
        if (message[0] == '\n')
            return;
        this->_wrenOutStream->info("[Script] {}", message);
    };

    _vm->setPrintFunc(logHandler);

    _vm->setPathResolveFunc(detail::ResolveImport);
    _vm->setLoadFileFunc(detail::LoadFile);
}

std::optional<std::string> ScriptingContext::RunScript(const std::string& path)
{
    try
    {
        _vm->runFromModule(path);
        return detail::ResolveImport(_vmInitConfig.includePaths, "", path);
    }
    catch (const wren::Exception& e)
    {
        bblog::error(e.what());
    }

    return std::nullopt;
}
