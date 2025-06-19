#pragma once

#include "log.hpp"
#include "physfs.hpp"
#include <chrono>
#include <fstream>
#include <optional>
#include <stb_image.h>
#include <string>
#include <vector>

/// @brief File interface system for opening and closing files
namespace fileIO
{
constexpr auto DEFAULT_READ_FLAGS = std::ios::in | std::ios::binary;
constexpr auto DEFAULT_WRITE_FLAGS = std::ios::out | std::ios::trunc | std::ios::binary;

constexpr auto TEXT_READ_FLAGS = std::ios::in;
constexpr auto TEXT_WRITE_FLAGS = std::ios::out | std::ios::trunc;

using FileTime = std::chrono::time_point<std::chrono::file_clock>;

/// <summary>
/// Open a file stream for reading. Specify 0 or std::ios::flags
/// </summary>
std::optional<PhysFS::ifstream> OpenReadStream(const std::string& path);

/// <summary>
/// Open a file stream for writing. Specify 0 or std::ios::flags
/// </summary>
std::optional<PhysFS::ofstream> OpenWriteStream(const std::string& path);

/// <summary>
/// Dumps all bytes of a stream into a vector
/// </summary>
std::vector<std::byte> DumpStreamIntoBytes(std::istream& stream);

/// <summary>
/// Dumps stream into a string
/// </summary>
std::string DumpStreamIntoString(std::istream& stream);

/// <summary>
/// Check if a file exists.
/// </summary>
bool Exists(const std::string& path);

/// <summary>
/// Creates a directory at the specified path, returns false if this failed
/// </summary>
bool MakeDirectory(const std::string& path);

float* LoadFloatImageFromIfstream(PhysFS::ifstream& file, int32_t* x, int32_t* y, int32_t* channels_in_file, int32_t desired_channels);
stbi_uc* LoadImageFromIfstream(PhysFS::ifstream& file, int32_t* x, int32_t* y, int32_t* channels_in_file, int32_t desired_channels);

void Init(bool useStandard);

void Deinit();

};