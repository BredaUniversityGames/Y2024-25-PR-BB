#include <bit>
#include <fileIO.hpp>
#include <filesystem>
#include <log.hpp>
#include <scripting_context.hpp>
#include <wren.hpp>

struct WrenCallbacks
{
    static void SystemWrite(WrenVM* _vm, const char* text)
    {
        auto* context = static_cast<ScriptingContext*>(wrenGetUserData(_vm));

        if (context->_wrenOutStream != nullptr)
        {
            *(context->_wrenOutStream) << text;
        }
    }

    static void ErrorHandler(
        MAYBE_UNUSED WrenVM* _vm,
        WrenErrorType errorType,
        const char* module,
        const int line,
        const char* msg)
    {
        switch (errorType)
        {
        case WREN_ERROR_COMPILE:
        {
            bblog::error("Wren Interpreter: in module {}, line {}: {}", module, line, msg);
            break;
        }
        case WREN_ERROR_STACK_TRACE:
        {
            bblog::error(">>> in module {}, line {}: {}", module, line, msg);
            break;
        }
        case WREN_ERROR_RUNTIME:
        {
            bblog::error("Wren Runtime: {}", msg);
            break;
        }
        }
    }

    static WrenForeignClassMethods BindForeignClass(
        MAYBE_UNUSED WrenVM* _vm,
        MAYBE_UNUSED const char* module,
        MAYBE_UNUSED const char* class_name)
    {
        return { nullptr, nullptr };
    }

    static WrenForeignMethodFn BindForeignMethod(
        MAYBE_UNUSED WrenVM* _vm,
        MAYBE_UNUSED const char* module,
        MAYBE_UNUSED const char* class_name,
        MAYBE_UNUSED bool is_static,
        MAYBE_UNUSED const char* signature)
    {
        return nullptr;
    }

    static WrenLoadModuleResult LoadModule(WrenVM* _vm, const char* name)
    {
        auto* context = static_cast<ScriptingContext*>(wrenGetUserData(_vm));
        WrenLoadModuleResult result {};

        if (std::optional<std::string> source = context->LoadModuleSource(name))
        {
            context->_loadedModulePaths.emplace_back(std::string(name));
            std::string* heap_string = new std::string(std::move(source.value()));

            result.source = heap_string->c_str();
            result.onComplete = &FreeModuleSource;
            result.userData = heap_string;
        }

        return result;
    }

    static void FreeModuleSource(MAYBE_UNUSED WrenVM* _vm, MAYBE_UNUSED const char* name, WrenLoadModuleResult result)
    {
        auto* source_str = static_cast<std::string*>(result.userData);
        delete source_str;
    }

    static const char* ImportHandler(WrenVM* _vm, const char* importer, const char* imported)
    {
        using FilePath = std::filesystem::path;

        auto MakeCString = [](const std::string& str)
        {
            // Unfortunately, we need to allocate and pass ownership of paths to wren
            size_t allocation_size = sizeof(char) * (str.size() + 1);
            char* ret_val = std::bit_cast<char*>(std::malloc(allocation_size)); // NOLINT

            std::memset(ret_val, 0, allocation_size);
            std::memcpy(ret_val, str.data(), allocation_size - 1);

            return ret_val;
        };

        auto* context = static_cast<ScriptingContext*>(wrenGetUserData(_vm));

        // First check if the file exists relative to current directory
        FilePath sourcePath = FilePath(importer).make_preferred();
        FilePath importPath = FilePath(imported).make_preferred();

        FilePath relativeImportPath = sourcePath.parent_path() / importPath;

        auto relativePath = relativeImportPath.string();
        if (fileIO::Exists(relativePath))
        {
            return MakeCString(relativePath);
        }

        for (auto& include_path : context->_includePaths)
        {
            FilePath tryPath = FilePath(include_path) / importPath;

            std::string path = tryPath.string();
            if (fileIO::Exists(path))
            {
                return MakeCString(path);
            }
        }

        // Otherwise, just return the import path unmodified
        return MakeCString(importPath.string());
    }
};

ScriptingContext::ScriptingContext(const VMMemoryConfig& mem)
{
    WrenConfiguration config;
    wrenInitConfiguration(&config);

    config.userData = this;

    config.writeFn = &WrenCallbacks::SystemWrite;
    config.errorFn = &WrenCallbacks::ErrorHandler;
    config.bindForeignMethodFn = &WrenCallbacks::BindForeignMethod;
    config.bindForeignClassFn = &WrenCallbacks::BindForeignClass;
    config.loadModuleFn = &WrenCallbacks::LoadModule;
    config.resolveModuleFn = &WrenCallbacks::ImportHandler;

    config.initialHeapSize = mem.initialHeapSize;
    config.minHeapSize = mem.minHeapSize;
    config.heapGrowthPercent = mem.heapGrowthPercent;

    _vm = wrenNewVM(&config);
}

ScriptingContext::~ScriptingContext()
{
    wrenFreeVM(_vm);
}

bool ScriptingContext::InterpretWrenModule(const std::string& path)
{
    if (auto source = LoadModuleSource(path))
    {
        // Paths that map to the same file need to map to the same string as well
        using FilePath = std::filesystem::path;
        std::string preferred_path = FilePath(path).make_preferred().string();

        auto result = wrenInterpret(_vm, preferred_path.c_str(), source->c_str()) == WREN_RESULT_SUCCESS;

        if (result)
        {
            _loadedModulePaths.emplace_back(preferred_path);
        }

        return result;
    }

    bblog::error("Wren compilation: could not interpret {}", path);
    return false;
}

void ScriptingContext::AddWrenIncludePath(const std::string& path)
{
    using FilePath = std::filesystem::path;

    std::string preferred_path = FilePath(path).make_preferred().string();
    auto equals = [&preferred_path](const auto& rhs)
    { return preferred_path == rhs; };

    auto it = std::find_if(_includePaths.begin(), _includePaths.end(), equals);

    if (it == _includePaths.end())
    {
        _includePaths.emplace_back(preferred_path);
    }
}

std::optional<std::string> ScriptingContext::LoadModuleSource(const std::string& modulePath)
{
    if (auto stream = fileIO::OpenReadStream(modulePath, fileIO::TEXT_READ_FLAGS))
    {
        auto file_data = fileIO::DumpFullStream(stream.value());
        return std::string { std::bit_cast<const char*>(file_data.data()), file_data.size() };
    }

    return std::nullopt;
}