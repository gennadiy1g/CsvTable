#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace bfs = boost::filesystem;

using OnProgress = std::function<void(int, int)>;

class FileLines
{
public:
    explicit FileLines(const bfs::path& filePath, OnProgress onProgress = OnProgress()); // Constructor
    virtual ~FileLines() = default;                                                      // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    FileLines(const FileLines& src) = delete;
    FileLines& operator=(const FileLines& rhs) = delete;

    /* No move operations because field mFileStream has inaccessible move constructor and
     * move assignement operator */
    FileLines(FileLines&& src) = delete;
    FileLines& operator=(FileLines&& rhs) = delete;

    std::size_t numLines() const
    {
        return mNumLines;
    };

    bool isNumLinesLimitReached() const
    {
        return mIsNumLinesLimitReached;
    }

    std::wstring getLine(std::size_t lineNum);

    void stopReading()
    {
        mIsCancelled = true;
    };

    void finishReading()
    {
        mThread.join();
    }

private:
    void checkInputFile();
    void getPositionsOfSampleLines();

    bfs::path mFilePath;
    std::size_t mNumLines { 0 }; // Number of lines in the file
    bfs::ifstream mFileStream;
    OnProgress mOnProgress;
    std::size_t mPrevSampleNum { std::numeric_limits<std::size_t>::max() };

    // Shared between this class and GUI
    std::atomic_bool mIsCancelled { false };
    std::atomic_bool mIsNumLinesLimitReached { false };

    // Shared between getPositionsOfSampleLines() and getLine(); read only by both.
    bfs::ifstream::pos_type mFileSize { 0 };

    // Shared between getPositionsOfSampleLines() and getLine(); written by at least one.
    std::vector<bfs::ifstream::pos_type> mPosSampleLine; // Positions of sample lines
    std::size_t mNumLinesBetweenSamples { 1 };           // Number of lines between successive sample lines
    decltype(mPosSampleLine) mPosBetweenSamples;         // Positions of lines between sample lines

    std::mutex mMutex;
    std::thread mThread;
};

using EscapedListSeparator = boost::escaped_list_separator<wchar_t, std::char_traits<wchar_t>>;
using LineTokenizer = boost::tokenizer<EscapedListSeparator, std::wstring::const_iterator, std::wstring>;

class TokenizedFileLines
{
public:
    explicit TokenizedFileLines(const bfs::path& filePath, OnProgress onProgress = OnProgress())
        : mFileLines(filePath, onProgress) {}; // Constructor
    virtual ~TokenizedFileLines() = default;   // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    TokenizedFileLines(const TokenizedFileLines& src) = delete;
    TokenizedFileLines& operator=(const TokenizedFileLines& rhs) = delete;

    /* No move operations because field mFileLines has deleted move constructor and
     * move assignement operator */
    TokenizedFileLines(TokenizedFileLines&& src) = delete;
    TokenizedFileLines& operator=(TokenizedFileLines&& rhs) = delete;

    void setTokenizerParams(wchar_t escape, wchar_t separator, wchar_t quote);

    std::size_t numLines() const
    {
        return mFileLines.numLines();
    };

    bool isNumLinesLimitReached() const
    {
        return mFileLines.isNumLinesLimitReached();
    }

    std::size_t numColumns()
    {
        return getTokenizedLine(0).size();
    };

    const std::vector<std::wstring>& getTokenizedLine(std::size_t lineNum);

    void stopReading()
    {
        mFileLines.stopReading();
    };

    void finishReading()
    {
        mFileLines.finishReading();
    }

private:
    FileLines mFileLines;
    EscapedListSeparator mEscapedListSeparator { L'\0', L',', L'\"' };
    std::map<std::size_t, std::vector<std::wstring>> mTokenizedLines;
    static constexpr std::size_t kMaxSize { 10'000 };
    wchar_t mSeparator { L',' };
    wchar_t mEscape { L'\0' };
    wchar_t mQuote { L'\"' };
};
