#pragma once

#include <boost/filesystem.hpp>
#include <optional>

namespace bfs = boost::filesystem;

void initLocalization();
void initLogging();

constexpr wchar_t kTab{L'\t'}, kPipe{L'|'}, kSemicolon{L';'}, kComma{L','}, kSpace{L' '}, kDoubleQuote{L'\"'},
    kSingleQuote{L'\''};

void detectSeparatorAndQuote(bfs::path filePath, std::optional<wchar_t> &separator, std::optional<wchar_t> &quote);
