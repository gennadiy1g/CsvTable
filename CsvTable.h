#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/locale.hpp>
#include <stdexcept>
#include <string>

namespace bfs = boost::filesystem;
namespace blocale = boost::locale;

using namespace std::literals::string_literals;

class FileLines
{
public:
    FileLines(const bfs::path& filePath); // Constructor
    virtual ~FileLines() = default;       // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    FileLines(const FileLines& src) = delete;
    FileLines& operator=(const FileLines& rhs) = delete;

    // Explicitly default move constructor and move assignment operator.
    FileLines(FileLines&& src) = default;
    FileLines& operator=(FileLines&& rhs) = default;

    void generateOffsets();
    std::wstring getLine(const int lineNumber);

protected:
private:
    bfs::wifstream mFileStream;
};
