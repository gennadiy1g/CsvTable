#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace bfs = boost::filesystem;

using OnProgress = std::function<void(int)>;

class FileLines {
public:
    explicit FileLines(const bfs::path& filePath, OnProgress onProgress = OnProgress()); // Constructor
    virtual ~FileLines() = default; // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    FileLines(const FileLines& src) = delete;
    FileLines& operator=(const FileLines& rhs) = delete;

    // Explicitly default move constructor and move assignment operator.
    FileLines(FileLines&& src) = default;
    FileLines& operator=(FileLines&& rhs) = default;

    std::size_t numLines() const { return mNumLines; };
    std::wstring getLine(std::size_t lineNum);

private:
    void checkInputFile();
    void getPositionsOfSampleLines();

    // The file
    bfs::path mFilePath;
    bfs::ifstream mFileStream;
    std::intmax_t mFileSize { 0 };

    std::size_t mNumLines { 0 }; // Number of lines in the file
    std::vector<std::size_t> mPosSampleLine; // Positions of sample lines
    std::size_t mNumLinesBetweenSamples { 1 }; // Number of lines between successive sample lines

    std::vector<std::size_t> mPosBetweenSamples; // Positions of lines between sample lines
    std::size_t mPrevSampleNum { std::numeric_limits<std::size_t>::max() };

    OnProgress mOnProgress { OnProgress() };
};

typedef boost::escaped_list_separator<wchar_t, std::char_traits<wchar_t>> EscapedListSeparator;
typedef boost::tokenizer<EscapedListSeparator, std::wstring::const_iterator, std::wstring> LineTokenizer;

class TokenizedFileLines {
public:
    explicit TokenizedFileLines(const bfs::path& filePath, OnProgress onProgress = OnProgress()); // Constructor
    virtual ~TokenizedFileLines() = default; // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    TokenizedFileLines(const TokenizedFileLines& src) = delete;
    TokenizedFileLines& operator=(const TokenizedFileLines& rhs) = delete;

    // Explicitly default move constructor and move assignment operator.
    TokenizedFileLines(TokenizedFileLines&& src) = default;
    TokenizedFileLines& operator=(TokenizedFileLines&& rhs) = default;

    void setTokenizerParams(wchar_t escape, wchar_t separator, wchar_t quote);
    std::size_t numLines() const { return mFileLines.numLines(); };
    std::size_t numColumns() { return getTokenizedLine(0).size(); };
    const std::vector<std::wstring>& getTokenizedLine(std::size_t lineNum);

    void clear();

private:
    FileLines mFileLines;
    EscapedListSeparator mEscapedListSeparator { L'\0', L',', L'\"' };
    std::map<std::size_t, std::vector<std::wstring>> mTokenizedLines;
    std::size_t kMaxSize { 10000 };
    wchar_t mSeparator { L',' };
    wchar_t mEscape { L'\0' };
    wchar_t mQuote { L'\"' };
};
