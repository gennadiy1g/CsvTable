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
    const std::size_t kMinNumRecords { 100 }; /* read at least that many records, excluding the line with headers,
     * before trying to evaluate the total number of records */

    const std::size_t kMaxNumSamples { 10000 }; // maximum number of offset samples

    std::size_t posAfterHeaderLine { 0 };
    std::size_t posAfterMinNumRecords { 0 };

    std::wstring line;
    while (std::getline(mFileStream, line)) {
        if (!(mNumLines % mNumLinesBetweenSamples)) {
            mOffsetsSamples.push_back(mFileStream.tellg());
        }

        if (!mNumLines) {
            // First line contains headers
            posAfterHeaderLine = mFileStream.tellg();
        } else if (mNumLines == kMinNumRecords /* do not count the line with headers */) {
            posAfterMinNumRecords = mFileStream.tellg();
            assert(posAfterMinNumRecords > 0);

            // Evaluate number of records in the file
            auto approxNumLines = kMinNumRecords * (bfs::file_size(mFilePath) - posAfterHeaderLine) / posAfterMinNumRecords;
            assert(approxNumLines > 0);

            // Calculate number of lines between offset samples
            mNumLinesBetweenSamples = lround(approxNumLines / kMaxNumSamples);
            assert(mNumLinesBetweenSamples >= 1);

            // Keep offsets for lines where line number is divisible by mNumLinesBetweenSamples, get rid of offsets for other lines
            if (mNumLinesBetweenSamples > 1) {
                std::vector<std::size_t> offsetsSamples;
                for (std::size_t i = 0; i < mOffsetsSamples.size(); i += mNumLinesBetweenSamples) {
                    offsetsSamples.push_back(mOffsetsSamples[i]);
                }
                std::swap(mOffsetsSamples, offsetsSamples);
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
    std::size_t offset { 0 };
    std::wstring line;

    if (mNumLinesBetweenSamples == 1) {
        assert(lineNum < mOffsetsSamples.size());
        offset = mOffsetsSamples.at(lineNum);
        mFileStream.seekg(offset);
        std::getline(mFileStream, line);
    } else {
        auto posOffsetSample = std::floor(lineNum / mNumLinesBetweenSamples); // pos of the closest offset sample
        assert(posOffsetSample < mOffsetsSamples.size());
        offset = mOffsetsSamples.at(posOffsetSample);
        mFileStream.seekg(offset);

        auto numLinesFromSampleLine = lineNum % mNumLinesBetweenSamples; /* Number of lines between the line of the closest offset sample and
         * the requested line lineNum */
        if (!numLinesFromSampleLine) {
            std::getline(mFileStream, line);
        } else {
            for (std::size_t i = 0; i < numLinesFromSampleLine; ++i) {
                std::getline(mFileStream, line);
            }
        }
    }
    return line;
}
