#include <boost/locale.hpp>
#include <stdexcept>

#include "CsvTable.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath)
    : mFilePath(filePath)
    , mFileStream(filePath)
{
    checkInputFile();

    if(!mFileStream.is_open()) {
        throw std::runtime_error(
            "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
    }
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

void FileLines::generateOffsets()
{
    const std::size_t kMinNumRecords { 100 }; /* read at least that many records before trying to evaluate
                                               * the total number of records */
    std::size_t posAfterHeaderLine { 0 };
    std::size_t posAfterMinNumRecords { 0 };

    std::wstring line;
    while(std::getline(mFileStream, line)) {
        ++mNumLines;
        if(mNumLines == 1) {
            posAfterHeaderLine = mFileStream.tellg();
        } else if(mNumLines == kMinNumRecords) {
            posAfterMinNumRecords = mFileStream.tellg();
        }
    }

    if(!mFileStream.eof()) {
        std::stringstream message;
        message << "Character set conversion error! File: \"" << blocale::conv::utf_to_utf<char>(mFilePath.native())
                << "\", line: " << mNumLines + 1 << ", column: " << line.length() + 1 << '.';
        throw std::runtime_error(message.str());
    }
}
