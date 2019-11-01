#include <boost/locale.hpp>
#include <stdexcept>

#include "CsvTable.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path& filePath)
    : mFileStream(filePath)
{
    if(!mFileStream.is_open() || mFileStream.fail()) {
        throw std::runtime_error(
            "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
    }
}
