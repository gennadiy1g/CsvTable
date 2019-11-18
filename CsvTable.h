#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <string>
#include <vector>

namespace bfs = boost::filesystem;

class FileLines {
public:
    FileLines(const bfs::path& filePath); // Constructor
    virtual ~FileLines() = default; // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    FileLines(const FileLines& src) = delete;
    FileLines& operator=(const FileLines& rhs) = delete;

    // Explicitly default move constructor and move assignment operator.
    FileLines(FileLines&& src) = default;
    FileLines& operator=(FileLines&& rhs) = default;

    void getPositionsOfSampleLines();
    std::size_t numLines() { return mNumLines; };
    std::wstring getLine(std::size_t lineNum);

protected:
private:
    void checkInputFile();

    // The file
    bfs::path mFilePath;
    bfs::ifstream mFileStream;

    std::size_t mNumLines { 0 }; // Number of lines in the file
    std::vector<std::size_t> mPosSampleLine; // Positions of sample lines
    std::size_t mNumLinesBetweenSamples { 1 }; // Number of lines between successive sample lines

    std::vector<std::size_t> mPosBetweenSamples; // Positions of lines between sample lines
    std::size_t mPrevLineNumNearSample { 0 };
};
