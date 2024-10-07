//
// Created by luuk on 25-9-2024.
//

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

struct FileWatcher{
    FileWatcher(const std::string_view path,const std::function<void(std::string)>& on_file_changed) : m_Path{path}, OnFileChanged(on_file_changed) {}
    explicit FileWatcher(const std::string_view path) : m_Path{path} {}
    std::string m_Path;

    std::filesystem::file_time_type lastWriteTime{};
    void PollForUpdates()
    {
        if(std::filesystem::exists(m_Path))
        {
            auto currentWriteTime = std::filesystem::last_write_time(m_Path);
            if (currentWriteTime != lastWriteTime) {
                OnFileChanged(m_Path);
                lastWriteTime = currentWriteTime;
            }
        }
    }
    std::function<void(std::string)> OnFileChanged;
    
};