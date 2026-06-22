#include "MpaUtils/Os/ClipboardUtils.h"

#include <algorithm>

#if defined(_WIN32)
#include "MpaUtils/SafeWindows.hpp"
#endif

namespace mpa {

namespace {

#if defined(_WIN32)
struct ClipboardDropFiles {
    DWORD pFiles = 0;
    POINT pt{};
    BOOL fNC = FALSE;
    BOOL fWide = TRUE;
};

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        result.data(),
        size,
        nullptr,
        nullptr);
    return result;
}

bool OpenClipboardWithRetry() {
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (OpenClipboard(nullptr)) {
            return true;
        }
        Sleep(10);
    }
    return false;
}
#endif

}  // namespace

std::string ReadClipboardText() {
#if defined(_WIN32)
    if (!OpenClipboardWithRetry()) {
        return {};
    }
    struct ClipboardGuard {
        ~ClipboardGuard() { CloseClipboard(); }
    } guard;

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data == nullptr) {
        return {};
    }
    auto* text = static_cast<const wchar_t*>(GlobalLock(data));
    if (text == nullptr) {
        return {};
    }
    std::wstring wide(text);
    GlobalUnlock(data);
    return WideToUtf8(wide);
#else
    return {};
#endif
}

bool SetClipboardText(const std::string& text) {
#if defined(_WIN32)
    if (!OpenClipboardWithRetry()) {
        return false;
    }
    struct ClipboardGuard {
        ~ClipboardGuard() { CloseClipboard(); }
    } guard;

    if (!EmptyClipboard()) {
        return false;
    }

    const auto wide = Utf8ToWide(text);
    const auto bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) {
        return false;
    }
    auto* target = static_cast<wchar_t*>(GlobalLock(memory));
    std::copy(wide.begin(), wide.end(), target);
    target[wide.size()] = L'\0';
    GlobalUnlock(memory);

    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) {
        GlobalFree(memory);
        return false;
    }
    return true;
#else
    (void)text;
    return false;
#endif
}

bool SetClipboardFiles(const std::vector<std::filesystem::path>& paths) {
#if defined(_WIN32)
    if (!OpenClipboardWithRetry()) {
        return false;
    }
    struct ClipboardGuard {
        ~ClipboardGuard() { CloseClipboard(); }
    } guard;

    if (!EmptyClipboard()) {
        return false;
    }

    std::wstring path_block;
    for (const auto& path : paths) {
        path_block += std::filesystem::absolute(path).wstring();
        path_block.push_back(L'\0');
    }
    path_block.push_back(L'\0');

    const auto bytes = sizeof(ClipboardDropFiles) + path_block.size() * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
    if (memory == nullptr) {
        return false;
    }

    auto* drop_files = static_cast<ClipboardDropFiles*>(GlobalLock(memory));
    drop_files->pFiles = sizeof(ClipboardDropFiles);
    drop_files->fWide = TRUE;
    auto* target =
        reinterpret_cast<wchar_t*>(
            reinterpret_cast<char*>(drop_files) + sizeof(ClipboardDropFiles));
    std::copy(path_block.begin(), path_block.end(), target);
    GlobalUnlock(memory);

    if (SetClipboardData(CF_HDROP, memory) == nullptr) {
        GlobalFree(memory);
        return false;
    }
    return true;
#else
    (void)paths;
    return false;
#endif
}

#if defined(_WIN32)
bool SetClipboardBitmap(HBITMAP bitmap) {
    if (bitmap == nullptr) {
        return false;
    }
    if (!OpenClipboardWithRetry()) {
        DeleteObject(bitmap);
        return false;
    }
    struct ClipboardGuard {
        ~ClipboardGuard() { CloseClipboard(); }
    } guard;

    if (!EmptyClipboard()) {
        DeleteObject(bitmap);
        return false;
    }

    if (SetClipboardData(CF_BITMAP, bitmap) == nullptr) {
        DeleteObject(bitmap);
        return false;
    }
    return true;
}
#endif

bool ClearClipboard() {
#if defined(_WIN32)
    if (!OpenClipboardWithRetry()) {
        return false;
    }
    const auto cleared = EmptyClipboard() != FALSE;
    CloseClipboard();
    return cleared;
#else
    return false;
#endif
}

}  // namespace mpa
