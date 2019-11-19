#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "CsvTable.h"
#include "log.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath)
    : mFilePath(filePath)
    , mFileStream(filePath, std::ios_base::binary)
{
    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mFilePath=" << mFilePath << FUNCTION_FILE_LINE;

    checkInputFile();

    if (!mFileStream.is_open()) {
        throw std::runtime_error(
            "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
    }
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
    std::string line;
    while (mFileStream) {
        if (!(mNumLines % mNumLinesBetweenSamples)) {
            mPosSampleLine.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLines=" << mNumLines << ", mPosSampleLine[" << mPosSampleLine.size() - 1
                                                     << "]=" << mPosSampleLine.at(mPosSampleLine.size() - 1) << FUNCTION_FILE_LINE;
        }

        if (!std::getline(mFileStream, line)) {
            break;
        }

        if (mNumLines == kMinNumLines + 1) {
            // Evaluate number of records in the file
            assert(mNumLines = mPosSampleLine.size() - 1);
            auto approxNumLines = kMinNumLines * (bfs::file_size(mFilePath) - mPosSampleLine.at(1)) / mPosSampleLine.at(mNumLines);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "file_size=" << bfs::file_size(mFilePath) << ", approxNumLines=" << approxNumLines << FUNCTION_FILE_LINE;
            assert(approxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = std::max(lround(approxNumLines / kMaxNumSamples), 1l);
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
}

std::wstring FileLines::getLine(std::size_t lineNum)
{
    assert(0 <= lineNum && lineNum < mNumLines);
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
        auto lineNumNearSample = std::floor(lineNum / mNumLinesBetweenSamples); // line number of the nearest sample
        auto rem = lineNum % mNumLinesBetweenSamples;
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", mNumLinesBetweenSamples=" << mNumLinesBetweenSamples
                                                 << ", lineNumNearSample=" << lineNumNearSample << ", rem=" << rem << FUNCTION_FILE_LINE;
        assert(lineNumNearSample < mPosSampleLine.size());

        if (mPrevLineNumNearSample != lineNumNearSample) {
            mPosBetweenSamples.clear();
            mPrevLineNumNearSample = lineNumNearSample;
        }

        if (!mPosBetweenSamples.size()) {
            auto pos = mPosSampleLine.at(lineNumNearSample);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", pos=" << pos << FUNCTION_FILE_LINE;

            mFileStream.seekg(pos);
            std::getline(mFileStream, line);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                     << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;

            mPosBetweenSamples.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                     << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;

            for (std::size_t i = 0; i < rem; ++i) {
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;

                if (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1) {
                    mPosBetweenSamples.push_back(mFileStream.tellg());
                    BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1
                                                             << "]=" << mPosBetweenSamples.at(mPosBetweenSamples.size() - 1) << FUNCTION_FILE_LINE;
                }
            }
        } else {
            assert(mPosBetweenSamples.size() <= mNumLinesBetweenSamples - 1);
            if (rem <= mPosBetweenSamples.size()) {
                auto pos = mPosBetweenSamples.at(rem - 1);
                mFileStream.seekg(pos);
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=[" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << "], tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
                if ((rem == mPosBetweenSamples.size()) && (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1)) {
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

                    if (mPosBetweenSamples.size() < mNumLinesBetweenSamples - 1) {
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
