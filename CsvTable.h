#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include <map>
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
    bool isPrepared() { return mIsPrepared; };
    std::size_t numLines();
    std::wstring getLine(std::size_t lineNum);

private:
    void checkInputFile();

    // The file
    bfs::path mFilePath;
    bfs::ifstream mFileStream;
    std::intmax_t mFileSize { 0 };

    bool mIsPrepared { false };
    std::size_t mNumLines { 0 }; // Number of lines in the file
    std::vector<std::size_t> mPosSampleLine; // Positions of sample lines
    std::size_t mNumLinesBetweenSamples { 1 }; // Number of lines between successive sample lines

    std::vector<std::size_t> mPosBetweenSamples; // Positions of lines between sample lines
    std::size_t mPrevSampleNum { 0 };
};

typedef boost::escaped_list_separator<wchar_t, std::char_traits<wchar_t>> EscapedListSeparator;

class TokenizedFileLines {
public:
    TokenizedFileLines(const bfs::path& filePath); // Constructor
    virtual ~TokenizedFileLines() = default; // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    TokenizedFileLines(const TokenizedFileLines& src) = delete;
    TokenizedFileLines& operator=(const TokenizedFileLines& rhs) = delete;

    // Explicitly default move constructor and move assignment operator.
    TokenizedFileLines(TokenizedFileLines&& src) = default;
    TokenizedFileLines& operator=(TokenizedFileLines&& rhs) = default;

    void getPositionsOfSampleLines() { mFileLines.getPositionsOfSampleLines(); };
    void setTokenizerParams(wchar_t escape = L'\\', wchar_t fieldSeparator = L',', wchar_t quote = L'\"');
    std::size_t numLines() { return mFileLines.numLines(); };
    std::size_t numColumns();
    const std::vector<std::wstring>& getTokenizedLine(std::size_t lineNum);

private:
    FileLines mFileLines;
    EscapedListSeparator mEscapedListSeparator;
    std::map<int, std::vector<std::wstring>> mTokenizedLines;
};
