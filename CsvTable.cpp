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
            mPositionOfSampleLine.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLines=" << mNumLines << ", mPositionOfSampleLine[" << mPositionOfSampleLine.size() - 1
                                                     << "]=" << mPositionOfSampleLine.at(mPositionOfSampleLine.size() - 1) << FUNCTION_FILE_LINE;
        }

        if (!std::getline(mFileStream, line)) {
            break;
        }

        if (mNumLines == kMinNumLines + 1) {
            // Evaluate number of records in the file
            assert(mNumLines = mPositionOfSampleLine.size() - 1);
            auto approxNumLines = kMinNumLines * (bfs::file_size(mFilePath) - mPositionOfSampleLine.at(1)) / mPositionOfSampleLine.at(mNumLines);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "file_size=" << bfs::file_size(mFilePath) << ", approxNumLines=" << approxNumLines << FUNCTION_FILE_LINE;
            assert(approxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = std::max(lround(approxNumLines / kMaxNumSamples), 1l);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLinesBetweenSamples=" << mNumLinesBetweenSamples << FUNCTION_FILE_LINE;

            // Keep positions only for line numbers divisible by mNumLinesBetweenSamples
            if (mNumLinesBetweenSamples > 1) {
                std::vector<std::size_t> keep;
                for (std::size_t i = 0; i < mPositionOfSampleLine.size(); i += mNumLinesBetweenSamples) {
                    keep.push_back(mPositionOfSampleLine[i]);
                }
                std::swap(mPositionOfSampleLine, keep);
                mPositionOfSampleLine.reserve(kMaxNumSamples);
                mOffsets.reserve(mNumLinesBetweenSamples - 1);
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
        assert(lineNum < mPositionOfSampleLine.size());
        auto pos = mPositionOfSampleLine.at(lineNum);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", pos=" << pos << FUNCTION_FILE_LINE;
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                 << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
    } else {
        auto lineNumNearSample = std::floor(lineNum / mNumLinesBetweenSamples); // line number of the nearest sample
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", mNumLinesBetweenSamples=" << mNumLinesBetweenSamples
                                                 << ", lineNumNearSample=" << lineNumNearSample << FUNCTION_FILE_LINE;

        if (mPrevLineNumNearSample != lineNumNearSample) {
            mOffsets.clear();
            mPrevLineNumNearSample = lineNumNearSample;
        }

        if (!mOffsets.size()) {
            assert(lineNumNearSample < mPositionOfSampleLine.size());
            auto pos = mPositionOfSampleLine.at(lineNumNearSample);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "lineNum=" << lineNum << ", pos=" << pos << FUNCTION_FILE_LINE;

            mFileStream.seekg(pos);
            std::getline(mFileStream, line);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                     << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            for (std::size_t i = 0; i < lineNum % mNumLinesBetweenSamples; ++i) {
                std::getline(mFileStream, line);
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=" << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line))
                                                         << ", tellg()=" << mFileStream.tellg() << FUNCTION_FILE_LINE;
            }
        }
    }
    return boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line));
}
