#include "MpaUtils/Process/ChildProcess.h"

#include <cwchar>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32)
#include "MpaUtils/SafeWindows.hpp"
#endif

namespace mpa {

namespace {

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const auto length = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0);
    if (length <= 0) {
        throw std::runtime_error("failed to convert utf8 to wide string");
    }

    std::wstring wide(static_cast<size_t>(length), L'\0');
    const auto written = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        wide.data(),
        length);
    if (written != length) {
        throw std::runtime_error("failed to convert utf8 to wide string");
    }
    return wide;
}

std::wstring QuoteArgument(std::string_view argument) {
    std::wstring result;
    result.push_back(L'"');
    for (const wchar_t ch : Utf8ToWide(std::string(argument))) {
        if (ch == L'"') {
            result.push_back(L'\\');
        }
        result.push_back(ch);
    }
    result.push_back(L'"');
    return result;
}

std::wstring BuildCommandLine(
    const std::string& executable,
    const std::vector<std::string>& arguments) {
    std::wstring command_line = QuoteArgument(executable);
    for (const auto& argument : arguments) {
        command_line.push_back(L' ');
        command_line.append(QuoteArgument(argument));
    }
    return command_line;
}

std::map<std::wstring, std::wstring> CurrentEnvironment() {
    auto* raw_environment = GetEnvironmentStringsW();
    if (raw_environment == nullptr) {
        throw std::runtime_error("failed to read current process environment");
    }

    std::map<std::wstring, std::wstring> environment;
    for (auto* entry = raw_environment; *entry != L'\0'; entry += std::wcslen(entry) + 1) {
        std::wstring line(entry);
        const auto separator = line.find(L'=', 1);
        if (separator == std::wstring::npos) {
            continue;
        }
        environment[line.substr(0, separator)] = line.substr(separator + 1);
    }
    FreeEnvironmentStringsW(raw_environment);
    return environment;
}

std::wstring BuildEnvironmentBlock(const std::map<std::string, std::string>& environment) {
    if (environment.empty()) {
        return {};
    }

    auto merged_environment = CurrentEnvironment();
    for (const auto& [key, value] : environment) {
        merged_environment[Utf8ToWide(key)] = Utf8ToWide(value);
    }

    std::wstring block;
    for (const auto& [key, value] : merged_environment) {
        block.append(key);
        block.push_back(L'=');
        block.append(value);
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    return block;
}

bool ValidNativeHandle(void* handle) {
    return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}

void CloseNativeHandle(void*& handle) {
    if (!ValidNativeHandle(handle)) {
        handle = nullptr;
        return;
    }
    CloseHandle(static_cast<HANDLE>(handle));
    handle = nullptr;
}
#endif

}  // namespace

ChildProcessHandle::ChildProcessHandle(ChildProcessHandle&& other) noexcept
    : pid(std::exchange(other.pid, 0)),
      native_process_handle(std::exchange(other.native_process_handle, nullptr)),
      native_thread_handle(std::exchange(other.native_thread_handle, nullptr)) {}

ChildProcessHandle& ChildProcessHandle::operator=(ChildProcessHandle&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    pid = std::exchange(other.pid, 0);
    native_process_handle = std::exchange(other.native_process_handle, nullptr);
    native_thread_handle = std::exchange(other.native_thread_handle, nullptr);
    return *this;
}

ChildProcessHandle StartChildProcess(const ChildProcessStartInfo& start_info) {
#if defined(_WIN32)
    if (start_info.executable.empty()) {
        throw std::runtime_error("child process executable is empty");
    }

    auto command_line = BuildCommandLine(start_info.executable, start_info.arguments);
    auto working_directory = start_info.working_directory.has_value()
        ? Utf8ToWide(*start_info.working_directory)
        : std::wstring{};
    auto environment_block = BuildEnvironmentBlock(start_info.environment);

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};

    DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
    if (start_info.hide_window) {
        creation_flags |= CREATE_NO_WINDOW;
    }

    const BOOL created = CreateProcessW(
        nullptr,
        command_line.data(),
        nullptr,
        nullptr,
        FALSE,
        creation_flags,
        environment_block.empty() ? nullptr : environment_block.data(),
        working_directory.empty() ? nullptr : working_directory.c_str(),
        &startup_info,
        &process_info);
    if (!created) {
        throw std::runtime_error("failed to create child process: " + start_info.executable);
    }

    ChildProcessHandle handle;
    handle.pid = static_cast<uint32_t>(process_info.dwProcessId);
    handle.native_process_handle = process_info.hProcess;
    handle.native_thread_handle = process_info.hThread;
    return handle;
#else
    (void)start_info;
    throw std::runtime_error("child process launch is not implemented on this platform");
#endif
}

bool IsChildProcessRunning(const ChildProcessHandle& handle) {
#if defined(_WIN32)
    if (!ValidNativeHandle(handle.native_process_handle)) {
        return false;
    }
    return WaitForSingleObject(static_cast<HANDLE>(handle.native_process_handle), 0) == WAIT_TIMEOUT;
#else
    (void)handle;
    return false;
#endif
}

void StopChildProcess(ChildProcessHandle& handle, uint32_t exit_code) {
#if defined(_WIN32)
    if (ValidNativeHandle(handle.native_thread_handle)) {
        CloseNativeHandle(handle.native_thread_handle);
    }

    if (ValidNativeHandle(handle.native_process_handle)) {
        auto* native = static_cast<HANDLE>(handle.native_process_handle);
        if (WaitForSingleObject(native, 0) == WAIT_TIMEOUT) {
            TerminateProcess(native, exit_code);
            WaitForSingleObject(native, 5000);
        }
        CloseNativeHandle(handle.native_process_handle);
    }
    handle.pid = 0;
#else
    (void)handle;
    (void)exit_code;
#endif
}

}  // namespace mpa
