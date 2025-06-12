#include "file_io.hpp"
#include <bit>
#include <filesystem>
#include <stb_image.h>

std::optional<PhysFS::ifstream> fileIO::OpenReadStream(const std::string& path)
{
    if (!PhysFS::exists(path))
    {
        return std::nullopt;
    }

    try
    {
        return std::optional<PhysFS::ifstream> { path };
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<PhysFS::ofstream> fileIO::OpenWriteStream(const std::string& path)
{
    if (!PhysFS::exists(path))
    {
        return std::nullopt;
    }

    try
    {
        return std::nullopt; // std::optional<PhysFS::ofstream> { path };
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool fileIO::Exists(const std::string& path)
{
    return PhysFS::exists(path);
}

bool fileIO::MakeDirectory(const std::string& path)
{
    return PhysFS::mkdir(path);
}

std::optional<fileIO::FileTime> fileIO::GetLastModifiedTime(const std::string& path)
{

    if (Exists(path))
    {
        // auto lastModTime = PhysFS::getLastModTime(path); // TODO: Figure out what unit this is and convert to chronos
        return fileIO::FileTime {};
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<std::byte> fileIO::DumpStreamIntoBytes(std::istream& stream)
{
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();
    std::vector<std::byte> out(size, {});
    stream.seekg(0);
    stream.read(std::bit_cast<char*>(out.data()), size);
    return out;
}

std::string fileIO::DumpStreamIntoString(std::istream& stream)
{
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();
    std::string out(size, {});
    stream.seekg(0);
    stream.read(std::bit_cast<char*>(out.data()), size);
    return out;
}

struct STBIStreamContext
{
    PhysFS::ifstream* stream;
};

// Callback to read data from the stream
int32_t stbi_read(void* user, char* data, int32_t size)
{
    auto* ctx = static_cast<STBIStreamContext*>(user);
    ctx->stream->read(data, size);
    return static_cast<int>(ctx->stream->gcount());
}

// Callback to skip bytes in the stream
void stbi_skip(void* user, int32_t n)
{
    auto* ctx = static_cast<STBIStreamContext*>(user);
    ctx->stream->clear(); // clear fail bits
    ctx->stream->seekg(n, std::ios::cur);
}

// Callback to check if end of stream is reached
int stbi_eof(void* user)
{
    auto* ctx = static_cast<STBIStreamContext*>(user);
    return ctx->stream->eof() ? 1 : 0;
}

// Load float image from ifstream
float* fileIO::LoadFloatImageFromIfstream(PhysFS::ifstream& file, int32_t* x, int32_t* y, int32_t* channels_in_file, int32_t desired_channels)
{
    STBIStreamContext ctx { &file };

    stbi_io_callbacks callbacks {
        stbi_read,
        stbi_skip,
        stbi_eof
    };

    return stbi_loadf_from_callbacks(&callbacks, &ctx, x, y, channels_in_file, desired_channels);
}

stbi_uc* fileIO::LoadImageFromIfstream(PhysFS::ifstream& file, int32_t* x, int32_t* y, int32_t* channels_in_file, int32_t desired_channels)
{
    STBIStreamContext ctx { &file };

    stbi_io_callbacks callbacks {
        stbi_read,
        stbi_skip,
        stbi_eof
    };

    return stbi_load_from_callbacks(&callbacks, &ctx, x, y, channels_in_file, desired_channels);
}
