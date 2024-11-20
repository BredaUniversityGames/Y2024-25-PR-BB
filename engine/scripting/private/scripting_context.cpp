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
            bblog::error("Wren Interpreter: {}", module, line, msg);
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
        auto MakeCString = [](const std::string& str)
        {
            // Unfortunately, we need to allocate and pass ownership of paths to wren
            size_t allocation_size = sizeof(char) * (str.size() + 1);
            char* ret_val = std::bit_cast<char*>(std::malloc(allocation_size));

            std::memcpy(ret_val, str.data(), allocation_size - 1);
            ret_val[str.size()] = '\0';

            return ret_val;
        };

        auto* context = static_cast<ScriptingContext*>(wrenGetUserData(_vm));

        // First check if the file exists relative to current directory
        std::filesystem::path sourcePath = std::string(importer);
        std::filesystem::path importPath = std::string(imported);

        std::filesystem::path relativeImportPath = sourcePath.parent_path() / importPath;

        auto relativePath = relativeImportPath.string();
        if (fileIO::Exists(relativePath))
        {
            return MakeCString(relativePath);
        }

        for (auto& include_path : context->_includePaths)
        {
            std::filesystem::path tryPath = std::filesystem::path(include_path) / importPath;

            std::string path = tryPath.string();
            if (fileIO::Exists(path))
            {
                return MakeCString(path);
            }
        }

        // Otherwise, just return the import path unmodified
        return imported;
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
        auto result = wrenInterpret(_vm, path.c_str(), source->c_str());
        return result == WREN_RESULT_SUCCESS;
    }

    bblog::error("Wren compilation: could not interpret {}", path);
    return false;
}

std::optional<std::string> ScriptingContext::LoadModuleSource(const std::string& modulePath)
{
    if (auto stream = fileIO::OpenReadStream(modulePath, fileIO::TEXT_READ_FLAGS))
    {
        auto file_data = fileIO::DumpFullStream(stream.value());

        auto* start = std::bit_cast<const char*>(file_data.data());
        auto* end = start + file_data.size();

        return std::string { start, end };
    }

    return std::nullopt;
}