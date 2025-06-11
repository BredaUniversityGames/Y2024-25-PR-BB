#include "file_io.hpp"
#include <bit>
#include <filesystem>

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
        return std::optional<PhysFS::ofstream> { path };
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
