#pragma once

#include <boost/filesystem.hpp>
#include <optional>

namespace bfs = boost::filesystem;

void initLocalization();
void initLogging();

void detectSeparatorAndQuote(bfs::path filePath, std::optional<wchar_t> &separator, std::optional<wchar_t> &quote);
