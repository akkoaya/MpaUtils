#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mpa {

struct ChildProcessStartInfo {
    std::string executable;
    std::vector<std::string> arguments;
    std::optional<std::string> working_directory;
    std::map<std::string, std::string> environment;
    bool hide_window = true;
};

struct ChildProcessHandle {
    uint32_t pid = 0;
    void* native_process_handle = nullptr;
    void* native_thread_handle = nullptr;

    ChildProcessHandle() = default;
    ChildProcessHandle(const ChildProcessHandle&) = delete;
    ChildProcessHandle& operator=(const ChildProcessHandle&) = delete;
    ChildProcessHandle(ChildProcessHandle&& other) noexcept;
    ChildProcessHandle& operator=(ChildProcessHandle&& other) noexcept;
};

ChildProcessHandle StartChildProcess(const ChildProcessStartInfo& start_info);
bool IsChildProcessRunning(const ChildProcessHandle& handle);
void StopChildProcess(ChildProcessHandle& handle, uint32_t exit_code = 1);

}  // namespace mpa
