#include "log.hpp"
#include <spdlog/sinks/rotating_file_sink.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/utsname.h>
#endif

std::string GetOSName()
{
#ifdef _WIN32
    double version = 0.0;
    NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW osInfo {};

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    if (NULL != RtlGetVersion)
    {
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        version = (double)osInfo.dwMajorVersion;
    }

    return "Windows " + std::to_string(version);
#else
    utsname name {};
    uname(&name);

    return std::string(name.sysname) + " " + std::string(name.release);
#endif
}

std::string SerializeTimePoint( const std::chrono::system_clock::time_point& time, const std::string& format)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm {};
    _gmtime64_s(&tm, &tt); // GMT (UTC)
    std::stringstream ss {};
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

void spdlog::StartWritingToFile()
{
    const std::string logFileDir = "logs/";
    const std::string logFileExtension = ".bblog";

    // TODO: Probably good to put the version of the game here as well when we have access to that
    const auto now = std::chrono::system_clock::now();
    const std::string logFileName = SerializeTimePoint(now, "%dd-%mm-%Yy_%Hh-%Mm-%OSs");

    const std::string fullName = logFileDir + logFileName + logFileExtension;

    constexpr size_t maxFileSize = 1048576 * 5;
    constexpr size_t maxFiles = 3;

    auto fileLogger = spdlog::rotating_logger_mt("bblog", fullName, maxFileSize, maxFiles);
    bblog::set_default_logger(fileLogger);

    bblog::flush_on(bblog::level::level_enum::trace); // Flush on everything
}

void spdlog::PrintOSName()
{
    bblog::info("Operating System: {}", GetOSName());
}
