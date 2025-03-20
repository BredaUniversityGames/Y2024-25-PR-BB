#include "log.hpp"
#include <spdlog/sinks/rotating_file_sink.h>

void spdlog::StartWritingToFile()
{

const std::string logFileDir = "logs/";
const std::string logFileExtension = ".bblog";

// TODO: Probably good to put the version of the game here as well when we have access to that
const auto now = std::chrono::system_clock::now();
const std::string logFileName = std::format("{:%dd-%mm-%Yy_%Hh-%Mm-%OSs}", now);

const std::string fullName = logFileDir + logFileName + logFileExtension;

constexpr size_t maxFileSize = 1048576 * 5;
constexpr size_t maxFiles = 3;

auto fileLogger = spdlog::rotating_logger_mt("bblog", fullName, maxFileSize, maxFiles);
bblog::set_default_logger(fileLogger);

bblog::flush_on(bblog::level::level_enum::trace); // Flush on everything
}
