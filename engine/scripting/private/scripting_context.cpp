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
    auto relative = (Filepath(parent) / Filepath(name)).lexically_normal().make_preferred();

    if (std::filesystem::exists(relative))
    {
        return relative.string();
    }

    for (const auto& path : paths)
    {

        auto composed = (Filepath(path) / Filepath(name).lexically_normal().make_preferred());
        if (std::filesystem::exists(composed))
        {
            return composed.string();
        }
    }

    return fileIO::CanonicalizePath(name);
}

std::string LoadFile(const std::string& path)
{
    if (auto stream = fileIO::OpenReadStream(path, fileIO::TEXT_READ_FLAGS))
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

    for (auto& include_dir : _vmInitConfig.includePaths)
    {
        include_dir = fileIO::CanonicalizePath(include_dir);
    }

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
    auto correctedPath = fileIO::CanonicalizePath(path);

    try
    {
        _vm->runFromModule(correctedPath);
        return detail::ResolveImport(_vmInitConfig.includePaths, "", correctedPath);
    }
    catch (const wren::Exception& e)
    {
        bblog::error(e.what());
    }

    return std::nullopt;
}
