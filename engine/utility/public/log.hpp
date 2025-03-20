#pragma once
#include <spdlog/spdlog.h>

namespace spdlog
{
// After this is called, the logs are written to a file, instead of the console.
void StartWritingToFile();
}

namespace bblog = spdlog;
