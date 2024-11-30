#include "scripting_context.hpp"
#include "file_io.hpp"
#include "log.hpp"

#include <filesystem>

namespace ScriptLoading
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
        if (std::filesystem::exists(relative))
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
}

ScriptingContext::ScriptingContext(const ScriptingInitConfig& info)
{
    std::vector<std::string> correctedPaths;
    for (const auto& include_dir : info.includePaths)
    {
        correctedPaths.emplace_back(fileIO::CanonicalizePath(include_dir));
    }

    _vm = std::make_unique<wren::VM>(
        correctedPaths,
        info.initialHeapSize,
        info.minHeapSize,
        info.heapGrowthPercent);

    _vm->setPrintFunc([this](const char* message)
        { *this->_wrenOutStream << message; });

    _vm->setPathResolveFunc(ScriptLoading::ResolveImport);
    _vm->setLoadFileFunc(ScriptLoading::LoadFile);
}

ScriptingContext::~ScriptingContext()
{
    _vm.reset();
}

std::optional<std::string> ScriptingContext::RunScript(const std::string& path)
{
    auto correctedPath = fileIO::CanonicalizePath(path);

    try
    {
        _vm->runFromModule(correctedPath);
        return correctedPath;
    }
    catch (const wren::Exception& e)
    {
        bblog::error(e.what());
    }

    return std::nullopt;
}
