#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <stdexcept>

#include "CsvTable.h"
#include "log.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath, OnProgress onProgress)
    : mFilePath(filePath)
    , mFileStream(filePath, std::ios_base::binary)
    , mOnProgress(onProgress)
{
    mFileSize = bfs::file_size(mFilePath);

    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mFilePath=" << mFilePath << FUNCTION_FILE_LINE;

    checkInputFile();

    if (!mFileStream.is_open()) {
        throw std::runtime_error(
            "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
    }

    getPositionsOfSampleLines();
}

void FileLines::checkInputFile()
{
    bfs::file_status inputFileStatus = bfs::status(mFilePath);

    if (!bfs::exists(inputFileStatus)) {
        throw std::runtime_error(
            "File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" does not exist!"s);
    }

    if (!bfs::is_regular_file(inputFileStatus)) {
        throw std::runtime_error(
            "File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" is not a regular file!"s);
    }

    if (bfs::file_size(mFilePath) == 0) {
        throw std::runtime_error("File \""s + blocale::conv::utf_to_utf<char>(mFilePath.native()) + "\" is empty!"s);
    }
}

void FileLines::getPositionsOfSampleLines()
{
    const std::size_t kMinNumLines { 100 }; /* read at least that many lines, excluding the line with headers,
     * before trying to evaluate the number of lines in the file */

    const std::size_t kMaxNumSamples { 10000 }; // maximum number of sample lines

    auto& gLogger = GlobalLogger::get();
    std::string line { "" };
    int prevPercent { -1 }, percent { 0 };

    while (mFileStream) {
        if (!(mNumLines % mNumLinesBetweenSamples)) {
            mPosSampleLine.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLines=" << mNumLines << ", mPosSampleLine[" << mPosSampleLine.size() - 1
                                                     << "]=" << mPosSampleLine.at(mPosSampleLine.size() - 1) << FUNCTION_FILE_LINE;
        }

        assert(mFileSize);
        if (mOnProgress) {
            percent = lround(static_cast<float>(mFileStream.tellg()) / mFileSize * 100);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "percent=" << percent << FUNCTION_FILE_LINE;
            if (percent - prevPercent >= 1) {
                mOnProgress(percent);
            }
            prevPercent = percent;
        }

        if (!std::getline(mFileStream, line)) {
            break;
        }

        if (mNumLines == kMinNumLines + 1) {
            // Evaluate number of records in the file
            assert(mNumLines == mPosSampleLine.size() - 1);
            auto approxNumLines = kMinNumLines * (mFileSize - mPosSampleLine.at(1)) / mPosSampleLine.at(mNumLines);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "file_size=" << mFileSize << ", approxNumLines=" << approxNumLines << FUNCTION_FILE_LINE;
            assert(approxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = std::max(lround(static_cast<float>(approxNumLines) / kMaxNumSamples), 1l);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLinesBetweenSamples=" << mNumLinesBetweenSamples << FUNCTION_FILE_LINE;

            // Keep positions only for line numbers divisible by mNumLinesBetweenSamples
            if (mNumLinesBetweenSamples > 1) {
                std::vector<std::size_t> keep;
                for (std::size_t i = 0; i < mPosSampleLine.size(); i += mNumLinesBetweenSamples) {
                    keep.push_back(mPosSampleLine[i]);
                }
                std::swap(mPosSampleLine, keep);
                mPosSampleLine.reserve(kMaxNumSamples);
                mPosBetweenSamples.reserve(mNumLinesBetweenSamples - 1);
            }
        }

        ++mNumLines;
    }
    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;

    if (!mFileStream.eof()) {
        std::stringstream message;
        message << "Character set conversion error! File: \"" << blocale::conv::utf_to_utf<char>(mFilePath.native())
                << "\", line: " << mNumLines + 1 << ", column: " << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line)).length() + 1 << '.';
        throw std::runtime_error(message.str());
    }

    if (mOnProgress) {
        mOnProgress(100);
    }
}

std::wstring FileLines::getLine(std::size_t lineNum)
{
    assert(lineNum < mNumLines);
    std::string line;

    if (mFileStream.fail()) {
        mFileStream.clear();
    }

    auto& gLogger = GlobalLogger::get();
    if (mNumLinesBetweenSamples == 1) {
        assert(lineNum < mPosSampleLine.size());
        auto pos = mPosSampleLine.at(lineNum);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", pos=" << pos << FUNCTION_FILE_LINE;
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                 << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
    } else {
        auto sampleNum = lineNum / mNumLinesBetweenSamples; // line number of the nearest sample
        auto rem = lineNum % mNumLinesBetweenSamples;
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", sampleNum=" << sampleNum << ", rem=" << rem << FUNCTION_FILE_LINE;
        assert(sampleNum < mPosSampleLine.size());

        if (mPrevSampleNum != sampleNum) {
            mPosBetweenSamples.clear();
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "Cleared mPosBetweenSamples";
            mPrevSampleNum = sampleNum;
        }

        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples.size()=" << mPosBetweenSamples.size() << FUNCTION_FILE_LINE;
        if (!mPosBetweenSamples.size()) {
            auto pos = mPosSampleLine.at(sampleNum);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "pos=" << pos << FUNCTION_FILE_LINE;
            mFileStream.seekg(pos);
            std::getline(mFileStream, line);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                     << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            if (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1 && mFileStream.tellg() < mFileSize) {
                mPosBetweenSamples.push_back(mFileStream.tellg());
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                         << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;
            }

            for (std::size_t i = 0; i < rem; ++i) {
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                if (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1 && mFileStream.tellg() < mFileSize) {
                    mPosBetweenSamples.push_back(mFileStream.tellg());
                    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                             << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;
                }
            }
        } else {
            assert(mPosBetweenSamples.size() <= mNumLinesBetweenSamples - 1);
            if (!rem) {
                auto pos = mPosSampleLine.at(sampleNum);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "pos=" << pos << FUNCTION_FILE_LINE;
                mFileStream.seekg(pos);
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            } else if (rem <= mPosBetweenSamples.size()) {
                auto pos = mPosBetweenSamples.at(rem - 1);
                mFileStream.seekg(pos);
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                if (rem == mPosBetweenSamples.size() && mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1 && mFileStream.tellg() < mFileSize) {
                    mPosBetweenSamples.push_back(mFileStream.tellg());
                    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                             << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;
                }
            } else {
                auto pos = mPosBetweenSamples.at(mPosBetweenSamples.size() - 1);
                for (std::size_t i = 0; i < rem - mPosBetweenSamples.size() + 1; ++i) {
                    mFileStream.seekg(pos);
                    std::getline(mFileStream, line);
                    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                             << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                    if (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1 && mFileStream.tellg() < mFileSize) {
                        mPosBetweenSamples.push_back(mFileStream.tellg());
                        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                                 << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;
                    }
                }
            }
        }
    }

    return boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line));
}

TokenizedFileLines::TokenizedFileLines(const bfs::path& filePath, OnProgress onProgress)
    : mFileLines(filePath, onProgress)
{
}

void TokenizedFileLines::setTokenizerParams(wchar_t escape, wchar_t fieldSeparator, wchar_t quote)
{
    mTokenizedLines.clear();
    mEscapedListSeparator = EscapedListSeparator(escape, fieldSeparator, quote);
}

const std::vector<std::wstring>& TokenizedFileLines::getTokenizedLine(std::size_t lineNum)
{
    auto& gLogger = GlobalLogger::get();
    auto search = mTokenizedLines.find(lineNum);
    if (search != mTokenizedLines.end()) {
        return search->second;
    } else {
        if (mTokenizedLines.size() == kMaxSize) {
            /* The size of the map is at maximum. Remove one element from the map - the element
             * that is furthest away from lineNum. */
            assert(mTokenizedLines.size() > 4); // the logic below works only if there are more than 4 elements in the map
            auto itFirst = mTokenizedLines.begin();
            if (itFirst->first == 0) {
                // Always keep the line #0 because it contains columns' names
                ++itFirst;
            }
            auto itLast = mTokenizedLines.rbegin();

            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "itFirst->first=" << itFirst->first << ", itLast->first=" << itLast->first << FUNCTION_FILE_LINE;
            if (std::imaxabs(lineNum - itFirst->first) > std::imaxabs(lineNum - itLast->first)) {
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "Erasing line #" << itFirst->first << FUNCTION_FILE_LINE;
                mTokenizedLines.erase(itFirst);
            } else {
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "Erasing line #" << itLast->first << FUNCTION_FILE_LINE;
                mTokenizedLines.erase(itLast->first);
            }
        }

        auto line = mFileLines.getLine(lineNum);
        LineTokenizer tok(line, mEscapedListSeparator);
        std::vector<std::wstring> tokenizedLine;
        for (auto beg = tok.begin(); beg != tok.end(); ++beg) {
            tokenizedLine.push_back(*beg);
        }
        const auto [it, success] = mTokenizedLines.insert({ lineNum, std::move(tokenizedLine) });
        assert(success);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "Inserted line #" << lineNum << FUNCTION_FILE_LINE;
        return it->second;
    }
}
