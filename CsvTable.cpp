#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <cassert>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <ratio>
#include <stdexcept>

#include "CsvTable.h"
#include "log.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath, OnProgress onProgress, IsCancelled isCancelled)
    : mFilePath(filePath)
    , mFileStream(filePath, std::ios_base::binary)
    , mPreviewMode(false)
    , mOnProgress(onProgress)
    , mIsCancelled(isCancelled)
{
    constructorHelper(filePath);
}

FileLines::FileLines(const bfs::path& filePath, std::size_t linesToPreview)
    : mFilePath(filePath)
    , mFileStream(filePath, std::ios_base::binary)
    , mPreviewMode(true)
    , mLinesToPreview(linesToPreview)
{
    constructorHelper(filePath);
}

void FileLines::constructorHelper(const bfs::path& filePath)
{
    BOOST_LOG_FUNCTION();

    checkInputFile();

    if(!mFileStream.is_open()) {
        throw std::runtime_error(
            "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
    }

    mFileSize = bfs::file_size(mFilePath);
    assert(mFileSize);
    getPositionsOfSampleLines();
}

void FileLines::checkInputFile()
{
    bfs::file_status inputFileStatus = bfs::status(mFilePath);

    if(!bfs::exists(inputFileStatus)) {
        throw std::runtime_error(
            "File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" does not exist!"s);
    }

    if(!bfs::is_regular_file(inputFileStatus)) {
        throw std::runtime_error(
            "File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" is not a regular file!"s);
    }

    if(bfs::file_size(mFilePath) == 0) {
        throw std::runtime_error("File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" is empty!"s);
    }
}

void FileLines::getPositionsOfSampleLines()
{
    BOOST_LOG_FUNCTION();

    constexpr std::size_t kMaxNumSamples { 10'000 }; // maximum number of sample lines, excluding headers' line

    auto& gLogger = GlobalLogger::get();
    std::string line;
    int prevPercent { -1 }, percent { 0 };
    auto prevTimePoint = std::chrono::system_clock::now();

    while(mFileStream.good()) {
        BOOST_LOG_NAMED_SCOPE("Reading the file");

        if(mPreviewMode && mNumLines == mLinesToPreview.value()) {
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "mPreviewMode=" << mPreviewMode << ", mNumLines=" << mNumLines
                                                  << ", mLinesToPreview=" << mLinesToPreview.value();
            break;
        };

        /* Class wxGrid uses int for number of rows. See int wxGridTableBase::GetRowsCount() const and virtual int
         * wxGridTableBase::GetNumberRows() at https://docs.wxwidgets.org/3.1.3/classwx_grid_table_base.html.
         * We do not need to get positions for more lines than the maximum number of rows that wxGrid can display. */
        if(mNumLines == static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            mIsNumLinesLimitReached = true;
            break;
        }

        if(!(mNumLines % mNumLinesBetweenSamples)) { // mNumLines does not include headers' line yet
            mPosSampleLine.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltriv::trace)
                << "mNumLines=" << mNumLines << ", mPosSampleLine[" << mPosSampleLine.size() - 1
                << "]=" << mPosSampleLine.at(mPosSampleLine.size() - 1);
        }

        if(mOnProgress) {
            percent = static_cast<int>(std::round(static_cast<float>(mFileStream.tellg()) / mFileSize * 100));
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "percent=" << percent;
            if(percent - prevPercent >= 1) {
                mOnProgress(percent);
                prevPercent = percent;
            }
        }

        if(mIsCancelled && mApproxNumLines) {
            const auto timePoint = std::chrono::system_clock::now();
            if(std::chrono::duration<float, std::milli>(timePoint - prevTimePoint).count() > 100) {
                if(mIsCancelled()) {
                    // Cancelled by user
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << "Cancelled by user";
                    mIsCancelled_ = true;
                    mApproxNumLines = calculateApproxNumLines();
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << "mApproxNumLines=" << mApproxNumLines;
                    break;
                }
                prevTimePoint = timePoint;
            }
        }

        if(!std::getline(mFileStream, line)) {
            break;
        }

        /* Read at least that many lines, excluding headers' line, before trying to evaluate the number of lines in the
         * file */
        constexpr std::size_t kMinNumLines { 100 };

        if(!mPreviewMode && mNumLines == kMinNumLines) {
            // Evaluate number of lines in the file, excluding headers' line
            assert(mFileStream && mFileStream.tellg() > 0);
            mApproxNumLines = calculateApproxNumLines();
            BOOST_LOG_SEV(gLogger, bltriv::trace)
                << "mNumLines=" << mNumLines << ", mFileSize=" << mFileSize << ", mApproxNumLines=" << mApproxNumLines;
            assert(mApproxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = std::max(std::lround(static_cast<float>(mApproxNumLines) / kMaxNumSamples), 1l);
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "mNumLinesBetweenSamples=" << mNumLinesBetweenSamples;

            // Keep positions only for line numbers divisible by mNumLinesBetweenSamples
            if(mNumLinesBetweenSamples > 1) {
                decltype(mPosSampleLine) keep;
                for(std::size_t i = 0; i < mPosSampleLine.size(); i += mNumLinesBetweenSamples) {
                    keep.push_back(mPosSampleLine[i]);
                }
                std::swap(mPosSampleLine, keep);
                mPosSampleLine.reserve(kMaxNumSamples + 1); // kMaxNumSamples data lines plus headers' line
                assert(mPosSampleLine.at(0) == 0);
                mPosBetweenSamples.reserve(mNumLinesBetweenSamples - 1);
            }
        }

        ++mNumLines; // mNumLines now includes headers' line
    }
    BOOST_LOG_SEV(gLogger, bltriv::trace) << "mFileStream.tellg()=" << mFileStream.tellg();

    if(!mFileStream.eof() && !mIsNumLinesLimitReached && !mIsCancelled_ && !mPreviewMode) {
        std::stringstream message;
        message << "Character set conversion error! File: \"" << blocale::conv::utf_to_utf<char>(mFilePath.native())
                << "\", line: " << mNumLines + 1
                << ", column: " << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line)).length() + 1 << '.';
        throw std::runtime_error(message.str());
    }

    mFileStream.clear();

    if(mOnProgress && percent < 100) {
        mOnProgress(100);
    }
}

std::wstring FileLines::getLine(std::size_t lineNum)
{
    BOOST_LOG_FUNCTION();
    assert(lineNum < mNumLines);
    std::string line;

    auto& gLogger = GlobalLogger::get();
    if(mNumLinesBetweenSamples == 1) {
        assert(lineNum < mPosSampleLine.size());
        auto pos = mPosSampleLine.at(lineNum);
        BOOST_LOG_SEV(gLogger, bltriv::trace) << "lineNum=" << lineNum << ", pos=" << pos;
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
        BOOST_LOG_SEV(gLogger, bltriv::trace)
            << "line.substr()=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
            << ", mFileStream.tellg()=" << mFileStream.tellg();
    } else {
        auto sampleNum = lineNum / mNumLinesBetweenSamples; // line number of the nearest sample
        auto rem = lineNum % mNumLinesBetweenSamples;
        BOOST_LOG_SEV(gLogger, bltriv::trace)
            << "lineNum=" << lineNum << ", sampleNum=" << sampleNum << ", rem=" << rem;
        assert(sampleNum < mPosSampleLine.size());

        if(mPrevSampleNum != sampleNum) {
            mPosBetweenSamples.clear();
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "Cleared mPosBetweenSamples";
            mPrevSampleNum = sampleNum;
        }

        auto morePosBetweenSamples = [this]() {
            return mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1 && mFileStream.tellg() < mFileSize;
        };

        BOOST_LOG_SEV(gLogger, bltriv::trace) << "mPosBetweenSamples.size()=" << mPosBetweenSamples.size();
        if(!mPosBetweenSamples.size()) {
            auto pos = mPosSampleLine.at(sampleNum);
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "pos=" << pos;
            mFileStream.seekg(pos);
            std::getline(mFileStream, line);
            BOOST_LOG_SEV(gLogger, bltriv::trace) << "line=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
                                                  << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            if(morePosBetweenSamples()) {
                mPosBetweenSamples.push_back(mFileStream.tellg());
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                      << "]=" << mPosBetweenSamples.back() << FUNCTION_FILE_LINE;
            }

            for(std::size_t i = 0; i < rem; ++i) {
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltriv::trace)
                    << "line=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
                    << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                if(morePosBetweenSamples()) {
                    mPosBetweenSamples.push_back(mFileStream.tellg());
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                          << "]=" << mPosBetweenSamples.back() << FUNCTION_FILE_LINE;
                }
            }
        } else {
            assert(mPosBetweenSamples.size() <= mNumLinesBetweenSamples - 1);
            if(!rem) {
                auto pos = mPosSampleLine.at(sampleNum);
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "pos=" << pos << FUNCTION_FILE_LINE;
                mFileStream.seekg(pos);
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltriv::trace)
                    << "line=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
                    << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            } else if(rem <= mPosBetweenSamples.size()) {
                auto pos = mPosBetweenSamples.at(rem - 1);
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "pos=" << pos << FUNCTION_FILE_LINE;
                mFileStream.seekg(pos);
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltriv::trace)
                    << "line=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
                    << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                if(rem == mPosBetweenSamples.size() && morePosBetweenSamples()) {
                    mPosBetweenSamples.push_back(mFileStream.tellg());
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                          << "]=" << mPosBetweenSamples.back() << FUNCTION_FILE_LINE;
                }
            } else {
                auto pos = mPosBetweenSamples.back();
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "pos=" << pos << FUNCTION_FILE_LINE;
                mFileStream.seekg(pos);
                auto reps = rem - mPosBetweenSamples.size() + 1; /* The last pos in mPosBetweenSamples is for the line
                  that has not been read yet, hence plus one. Do not eliminate varible reps by putting the expression
                  directly into the loop's condition, because size of mPosBetweenSamples changes in the loop's body.  */
                for(std::size_t i = 0; i < reps; ++i) {
                    std::getline(mFileStream, line);
                    BOOST_LOG_SEV(gLogger, bltriv::trace)
                        << "line=" << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)
                        << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                    if(morePosBetweenSamples()) {
                        mPosBetweenSamples.push_back(mFileStream.tellg()); // changes size of mPosBetweenSamples!
                        BOOST_LOG_SEV(gLogger, bltriv::trace)
                            << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                            << "]=" << mPosBetweenSamples.back() << FUNCTION_FILE_LINE;
                    }
                }
            }
        }
    }
    return boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line));
}

void TokenizedFileLines::setTokenizerParams(wchar_t escape, wchar_t separator, wchar_t quote)
{
    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;

    if(separator != mSeparator || quote != mQuote || escape != mEscape) {
        BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
        clear();
    }

    mEscapedListSeparator = EscapedListSeparator(escape, separator, quote);
    mEscape = escape;
    mSeparator = separator;
    mQuote = quote;

    BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
}

const std::vector<std::wstring>& TokenizedFileLines::getTokenizedLine(std::size_t lineNum)
{
    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltriv::trace) << "lineNum=" << lineNum << FUNCTION_FILE_LINE;

    auto search = mTokenizedLines.find(lineNum);
    if(search != mTokenizedLines.end()) {
        BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
        return search->second;
    } else {
        BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
        if(mTokenizedLines.size() == kMaxSize) {
            /* The size of the map is at maximum. Remove one element from the map - the element
             * that is furthest away from lineNum. */
            assert(
                mTokenizedLines.size() > 4); // the logic below works only if there are more than 4 elements in the map
            auto itFirst = mTokenizedLines.begin();
            if(itFirst->first == 0) {
                // Always keep the line #0 because it contains columns' names
                ++itFirst;
            }
            auto itLast = mTokenizedLines.rbegin();

            BOOST_LOG_SEV(gLogger, bltriv::trace)
                << "itFirst->first=" << itFirst->first << ", itLast->first=" << itLast->first << FUNCTION_FILE_LINE;
            if(std::imaxabs(lineNum - itFirst->first) > std::imaxabs(lineNum - itLast->first)) {
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "Erasing line #" << itFirst->first << FUNCTION_FILE_LINE;
                mTokenizedLines.erase(itFirst);
            } else {
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "Erasing line #" << itLast->first << FUNCTION_FILE_LINE;
                mTokenizedLines.erase(itLast->first);
            }
        }

        auto line = mFileLines.getLine(lineNum);
        BOOST_LOG_SEV(gLogger, bltriv::trace) << "line=" << line.substr(0, 50) << FUNCTION_FILE_LINE;
        LineTokenizer tok(line, mEscapedListSeparator);
        std::vector<std::wstring> tokenizedLine;
        for(auto beg = tok.begin(); beg != tok.end(); ++beg) {
            tokenizedLine.push_back(*beg);
        }
        const auto [it, success] = mTokenizedLines.insert({ lineNum, std::move(tokenizedLine) });
        assert(success);
        BOOST_LOG_SEV(gLogger, bltriv::trace) << "Inserted line #" << lineNum << FUNCTION_FILE_LINE;
        return it->second;
    }
}

void TokenizedFileLines::clear()
{
    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
    mTokenizedLines.clear();
}
