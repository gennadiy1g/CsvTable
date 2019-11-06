#include <boost/locale.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "CsvTable.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath)
    : mFilePath(filePath)
    , mFileStream(filePath)
{
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

void FileLines::generateOffsetsSamples()
{
    const std::size_t kMinNumLines { 100 }; /* read at least that many lines, excluding the line with headers,
     * before trying to evaluate the number of lines in the file */

    const std::size_t kMaxNumSamples { 10000 }; // maximum number of sample lines

    std::size_t posAfterHeaderLine { 0 }; // position after the line with headers
    std::size_t posAfterMinNumRecords { 0 }; // position after kMinNumLines

    std::wstring line;
    while (std::getline(mFileStream, line)) {
        if (!(mNumLines % mNumLinesBetweenSamples)) {
            mSamples.push_back(mFileStream.tellg());
        }

        if (!mNumLines) {
            // First line contains headers
            posAfterHeaderLine = mFileStream.tellg();
        } else if (mNumLines == kMinNumLines /* do not count the line with headers */) {
            posAfterMinNumRecords = mFileStream.tellg();
            assert(posAfterMinNumRecords > 0);

            // Evaluate number of records in the file
            auto approxNumLines = kMinNumLines * (bfs::file_size(mFilePath) - posAfterHeaderLine) / posAfterMinNumRecords;
            assert(approxNumLines > 0);

            // Calculate the number of lines between successive samples
            mNumLinesBetweenSamples = lround(approxNumLines / kMaxNumSamples);
            assert(mNumLinesBetweenSamples >= 1);

            // Keep positions only for lines with line number divisible by mNumLinesBetweenSamples
            if (mNumLinesBetweenSamples > 1) {
                std::vector<std::size_t> samples;
                for (std::size_t i = 0; i < mSamples.size(); i += mNumLinesBetweenSamples) {
                    samples.push_back(mSamples[i]);
                }
                std::swap(mSamples, samples);
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
        assert(lineNum < mSamples.size());
        pos = mSamples.at(lineNum);
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
    } else {
        auto indexNearestSample = std::floor(lineNum / mNumLinesBetweenSamples); // index of the nearest sample
        assert(indexNearestSample < mSamples.size());
        pos = mSamples.at(indexNearestSample);
        mFileStream.seekg(pos);

        auto numLinesFromNearestSample = lineNum % mNumLinesBetweenSamples; /* Number of lines between the nearest sample and
         * the requested line lineNum */
        if (!numLinesFromNearestSample) {
            std::getline(mFileStream, line);
        } else {
            for (std::size_t i = 0; i < numLinesFromNearestSample; ++i) {
                std::getline(mFileStream, line);
            }
        }
    }
    return line;
}
