#pragma once

#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
#include "MpaUtils/SafeWindows.hpp"
#endif

namespace mpa {

std::string ReadClipboardText();
bool SetClipboardText(const std::string& text);
bool SetClipboardFiles(const std::vector<std::filesystem::path>& paths);
#if defined(_WIN32)
// Transfers ownership of the bitmap handle to the clipboard on success and
// deletes it on failure.
bool SetClipboardBitmap(HBITMAP bitmap);
#endif
bool ClearClipboard();

}  // namespace mpa
