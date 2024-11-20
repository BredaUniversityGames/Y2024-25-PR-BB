#include <fileIO.hpp>
#include <filesystem>

std::optional<std::ifstream> fileIO::OpenReadStream(const std::string& path,
    std::ios::openmode flags)
{
    std::ifstream stream(path, flags);
    if (stream)
    {
        return stream;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<std::ofstream> fileIO::OpenWriteStream(const std::string& path,
    std::ios::openmode flags)
{
    std::ofstream stream(path, flags);
    if (stream.is_open())
    {
        return stream;
    }
    else
    {
        return std::nullopt;
    }
}

bool fileIO::Exists(const std::string& path)
{
    return std::filesystem::exists(path);
}

bool fileIO::MakeDirectory(const std::string& path)
{
    std::error_code e {};
    std::filesystem::create_directory(path, e);
    if (e == std::error_code {})
        return true;
    else
        return false;
}

std::optional<fileIO::FileTime> fileIO::GetLastModifiedTime(const std::string& path)
{
    if (Exists(path))
    {
        return std::filesystem::last_write_time(path);
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<std::byte> fileIO::DumpFullStream(std::istream& stream)
{
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();
    std::vector<std::byte> out(size, {});
    stream.seekg(0);
    stream.read(std::bit_cast<char*>(out.data()), size);
    return out;
}