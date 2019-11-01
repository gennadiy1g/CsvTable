#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string>

namespace bfs = boost::filesystem;

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
