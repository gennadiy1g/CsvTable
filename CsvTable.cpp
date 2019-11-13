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
    , mFileStream(filePath)
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
    std::wstring line;
    while (std::getline(mFileStream, line)) {
        if (!(mNumLines % mNumLinesBetweenSamples)) {
            mPositionOfSampleLine.push_back(mFileStream.tellg());
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLines=" << mNumLines << ", mPositionOfSampleLine[" << mPositionOfSampleLine.size() - 1
                                                     << "]=" << mPositionOfSampleLine.at(mPositionOfSampleLine.size() - 1) << FUNCTION_FILE_LINE;
        }

        if (mNumLines == kMinNumLines) {
            // Evaluate number of records in the file
            auto approxNumLines = kMinNumLines * (bfs::file_size(mFilePath) - mPositionOfSampleLine.at(0)) / mPositionOfSampleLine.at(kMinNumLines);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "file_size=" << bfs::file_size(mFilePath) << ", approxNumLines=" << approxNumLines << FUNCTION_FILE_LINE;
            assert(approxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = lround(approxNumLines / kMaxNumSamples);
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << "mNumLinesBetweenSamples=" << mNumLinesBetweenSamples << FUNCTION_FILE_LINE;
            assert(mNumLinesBetweenSamples >= 1);

            // Keep positions only for line numbers divisible by mNumLinesBetweenSamples
            if (mNumLinesBetweenSamples > 1) {
                std::vector<std::size_t> keep;
                for (std::size_t i = 0; i < mPositionOfSampleLine.size(); i += mNumLinesBetweenSamples) {
                    keep.push_back(mPositionOfSampleLine[i]);
                }
                std::swap(mPositionOfSampleLine, keep);
            }
        }

        ++mNumLines;
    }

    if (!mFileStream.eof()) {
        std::stringstream message;
        message << "Character set conversion error! File: \"" << blocale::conv::utf_to_utf<char>(mFilePath.native())
                << "\", line: " << mNumLines + 1 << ", column: " << line.length() + 1 << '.';
        throw std::runtime_error(message.str());
    }
}

std::wstring FileLines::getLine(std::size_t lineNum)
{
    assert(0 <= lineNum && lineNum < mNumLines);
    std::size_t pos { 0 };
    std::wstring line;

    if (mNumLinesBetweenSamples == 1) {
        assert(lineNum < mPositionOfSampleLine.size());
        pos = mPositionOfSampleLine.at(lineNum);
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
    } else {
        auto lineNumNearSample = std::floor(lineNum / mNumLinesBetweenSamples); // line number of the nearest sample
        assert(lineNumNearSample < mPositionOfSampleLine.size());
        pos = mPositionOfSampleLine.at(lineNumNearSample);
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);

        auto offNearSample = lineNum % mNumLinesBetweenSamples; /* offset between the nearest sample
         * and the requested line */
        if (offNearSample) {
            for (std::size_t i = 0; i < offNearSample; ++i) {
                std::getline(mFileStream, line);
            }
        }
    }
    return line;
}
