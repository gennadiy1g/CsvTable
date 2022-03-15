#pragma once

#include <boost/filesystem.hpp>
#include <optional>
#include <utility>

namespace bfs = boost::filesystem;

void initLocalization();
void initLogging();

std::pair<std::optional<wchar_t>, std::optional<wchar_t>> detectSeparatorAndQuote(const bfs::path &filePath);
