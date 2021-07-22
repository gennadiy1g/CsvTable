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
using IsCancelled = std::function<bool()>;

class FileLines
{
public:
    explicit FileLines(const bfs::path& filePath,
        OnProgress onProgress = OnProgress(),
        IsCancelled isCancelled = IsCancelled()); // Constructor
    FileLines(const bfs::path& filePath, std::size_t linesToPreview);
    virtual ~FileLines() = default; // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    FileLines(const FileLines& src) = delete;
    FileLines& operator=(const FileLines& rhs) = delete;

    std::size_t numLines() const
    {
        return mNumLines;
    };
    std::size_t approxNumLines() const
    {
        return mApproxNumLines;
    };
    bool isNumLinesLimitReached() const
    {
        return mIsNumLinesLimitReached;
    }
    std::wstring getLine(std::size_t lineNum);
    bool isCancelled() const
    {
        return mIsCancelled_;
    };

private:
    void constructorHelper(const bfs::path& filePath);
    void checkInputFile();
    void getPositionsOfSampleLines();
    std::size_t calculateApproxNumLines()
    {
        return mNumLines * (mFileSize - mPosSampleLine.at(1)) / (mFileStream.tellg() - mPosSampleLine.at(1));
    };

    // The file
    bfs::path mFilePath;
    bfs::ifstream mFileStream;
    bfs::ifstream::pos_type mFileSize { 0 };

    /* In the "preview" mode, only top lines of a file are scanned to show these top lines as quickly as possible in
     * the grid without waiting for the whole file to be scanned. In the "normal" mode, all lines of the file are
     * scanned.
     */
    bool mPreviewMode { false };
    std::optional<std::size_t> mLinesToPreview;

    std::size_t mNumLines { 0 };                         // Number of lines in the file
    std::size_t mApproxNumLines { 0 };                   // Approximate number of lines in the file
    std::vector<bfs::ifstream::pos_type> mPosSampleLine; // Positions of sample lines
    std::size_t mNumLinesBetweenSamples { 1 };           // Number of lines between successive sample lines
    bool mIsNumLinesLimitReached { false };

    decltype(mPosSampleLine) mPosBetweenSamples; // Positions of lines between sample lines
    std::size_t mPrevSampleNum { std::numeric_limits<std::size_t>::max() };

    OnProgress mOnProgress;
    IsCancelled mIsCancelled;
    bool mIsCancelled_ { false };
};

using EscapedListSeparator = boost::escaped_list_separator<wchar_t, std::char_traits<wchar_t>>;
using LineTokenizer = boost::tokenizer<EscapedListSeparator, std::wstring::const_iterator, std::wstring>;

class TokenizedFileLines
{
public:
    explicit TokenizedFileLines(const bfs::path& filePath,
        OnProgress onProgress = OnProgress(),
        IsCancelled isCancelled = IsCancelled())
        : mFileLines(filePath, onProgress, isCancelled) {}; // Constructor
    TokenizedFileLines(const bfs::path& filePath, std::size_t linesToPreview)
        : mFileLines(filePath, linesToPreview) {}; // Constructor
    virtual ~TokenizedFileLines() = default;       // Defaulted virtual destructor

    // Disallow assignment and pass-by-value.
    TokenizedFileLines(const TokenizedFileLines& src) = delete;
    TokenizedFileLines& operator=(const TokenizedFileLines& rhs) = delete;

    void setTokenizerParams(wchar_t escape, wchar_t separator, wchar_t quote);
    std::size_t numLines() const
    {
        return mFileLines.numLines();
    };
    std::size_t approxNumLines() const
    {
        return mFileLines.approxNumLines();
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
    bool isCancelled() const
    {
        return mFileLines.isCancelled();
    };

    void clear();

private:
    FileLines mFileLines;
    EscapedListSeparator mEscapedListSeparator { L'\0', L',', L'\"' };
    std::map<std::size_t, std::vector<std::wstring>> mTokenizedLines;
    std::size_t kMaxSize { 10'000 };
    wchar_t mSeparator { L',' };
    wchar_t mEscape { L'\0' };
    wchar_t mQuote { L'\"' };
};
