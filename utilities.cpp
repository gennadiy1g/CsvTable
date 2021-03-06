#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <string>
#include <string_view>

#include "log.h"
#include "utilities.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

void initLocalization()
{
#ifdef __WIN64__
    // Get global backend, and select winapi backend as default for all categories
    blocale::localization_backend_manager::global().select("winapi");
#endif

    /* Create and install global locale. Non UTF-8 encodings are not supported by winapi backend.
     * https://www.boost.org/doc/libs/1_69_0/libs/locale/doc/html/using_localization_backends.html */
    std::locale::global(blocale::generator().generate("en_US.UTF-8"));

    // This is needed to prevent C library to
    // convert strings to narrow
    // instead of C++ on some platforms
    std::ios_base::sync_with_stdio(false);

    // Use the new global locale for future wide character output
    // https://en.cppreference.com/w/cpp/locale/locale
    std::wcout.imbue(std::locale());
}

void initLogging()
{
    // clang-format off
    blog::add_file_log(
#ifndef DEBUG
        blkeywords::file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
        blkeywords::target_file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
#else
        blkeywords::file_name = "trace.log",
        blkeywords::target_file_name = "trace.log",
#endif

    blkeywords::format = (blexpressions::stream
        << blexpressions::attr<unsigned int>("LineID") << ' ' << bltrivial::severity << ' '
        << blexpressions::format_date_time<boost::posix_time::ptime>("TimeStamp", " %Y-%m-%d %H:%M:%S.%f ")
        << blexpressions::attr<blog::thread_id>("ThreadID") << ' ' << blexpressions::message),

    blkeywords::auto_flush = true);
    // clang-format on
    blog::add_common_attributes();

#ifndef DEBUG
    blog::core::get()->set_filter(bltrivial::severity >= bltrivial::info);
#endif
}

void detectSeparatorAndQuote(bfs::path filePath, std::optional<wchar_t>& separator, std::optional<wchar_t>& quote)
{
    auto& gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;

    separator = std::nullopt;
    quote = std::nullopt;

    std::wstring line { L"" };
    {
        bfs::wifstream fileStream(filePath);
        if (!fileStream) {
            throw std::runtime_error("Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
        }

        std::getline(fileStream, line);
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "line=" << line.substr(0, 50) << FUNCTION_FILE_LINE;
    }
    boost::trim(line);

    if (line.length()) {
        // Detect separator
        if (line.find(L'\t') != std::wstring::npos) {
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
            separator = L'\t';
        } else {
            auto ambiguous { false };
            for (auto& ch : line) {
                if (ch == L'|' || ch == L';' || ch == L',') {
                    if (!separator) {
                        BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
                        separator = ch;
                    } else {
                        if (separator.value() != ch) {
                            // Ambiguous situation - multiple separators found
                            BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
                            ambiguous = true;
                            separator = std::nullopt;
                            break;
                        }
                    }
                }
            }
            if (!ambiguous && !separator) {
                if (line.find(L' ') != std::wstring::npos) {
                    BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
                    separator = L' ';
                }
            }
        }

        // Detect quote
        if (line.front() == L'\"' || line.back() == L'\"') {
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
            quote = L'\"';
        } else if (line.front() == L'\'' || line.back() == L'\'') {
            BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
            quote = L'\'';
        }

        if (separator && !quote) {
            auto match = [](std::wstring_view line, wchar_t separator, wchar_t quote) {
                std::wstring patternLeft {};
                patternLeft += separator;
                patternLeft += quote;
                std::wstring patternRight {};
                patternRight += quote;
                patternRight += separator;
                return line.find(patternLeft) != std::wstring::npos && line.find(patternRight) != std::wstring::npos;
            };

            if (match(line, separator.value(), L'\"')) {
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
                quote = L'\"';
            } else if (match(line, separator.value(), L'\'')) {
                BOOST_LOG_SEV(gLogger, bltrivial::trace) << FUNCTION_FILE_LINE;
                quote = L'\'';
            }
        }
    }
}
