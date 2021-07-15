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
    // Get global backend
    /*static*/ auto bm = blocale::localization_backend_manager::global();

    // Select winapi backend as default for all categories
    bm.select("winapi");

    // Set this backend globally
    blocale::localization_backend_manager::global(bm);
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
        blkw::file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
        blkw::target_file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
#else
        blkw::file_name = "trace.log",
        blkw::target_file_name = "trace.log",
#endif

    blkw::format = (blexpr::stream
        << blexpr::format_date_time<boost::posix_time::ptime>("TimeStamp", " %Y-%m-%d %H:%M:%S.%f ")
//        << blexpr::attr<unsigned int>("LineID") << ' '
        << blexpr::attr<blog::thread_id>("ThreadID") << ' '
        << bltriv::severity << ' '
        << blexpr::format_named_scope("Scope", blkw::format = "%n (%f:%l)") << ' '
        << blexpr::message),

    blkw::auto_flush = true);
    // clang-format on
    blog::add_common_attributes();
    blog::core::get()->add_global_attribute("Scope", blattr::named_scope());

#ifndef DEBUG
    blog::core::get()->set_filter(bltriv::severity >= bltriv::info);
#endif
}

void detectSeparatorAndQuote(bfs::path filePath, std::optional<wchar_t>& separator, std::optional<wchar_t>& quote)
{
    BOOST_LOG_FUNCTION();
    auto& gLogger = GlobalLogger::get();

    separator = std::nullopt;
    quote = std::nullopt;

    std::wstring line { L"" };
    {
        BOOST_LOG_NAMED_SCOPE("Read 1st line");
        bfs::wifstream fileStream(filePath);
        if(!fileStream) {
            throw std::runtime_error(
                "Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" for reading!"s);
        }

        std::getline(fileStream, line);
        BOOST_LOG_SEV(gLogger, bltriv::trace) << "line=" << line;
    }

    boost::trim(line);
    if(line.length()) {
        constexpr wchar_t kTab { L'\t' }, kPipe { L'|' }, kSemicolon { L';' }, kComma { L',' }, kSpace { L' ' },
            kDoubleQuote { L'\"' }, kSingleQuote { L'\'' };
        {
            // Detect separator
            BOOST_LOG_NAMED_SCOPE("Detect separator");
            if(line.find(kTab) != std::wstring::npos) {
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "separator=\\t";
                separator = kTab;
            } else {
                bool ambiguous { false };
                for(auto const& ch : line) {
                    if(ch == kPipe || ch == kSemicolon || ch == kComma) {
                        if(!separator) {
                            BOOST_LOG_SEV(gLogger, bltriv::trace) << "separator=" << ch;
                            separator = ch;
                        } else {
                            if(separator.value() != ch) {
                                // Ambiguous situation - multiple separators found
                                BOOST_LOG_SEV(gLogger, bltriv::trace) << "Ambiguous! Another separator=" << ch;
                                ambiguous = true;
                                separator = std::nullopt;
                                break;
                            }
                        }
                    }
                }
                if(!ambiguous && !separator) {
                    if(line.find(kSpace) != std::wstring::npos) {
                        BOOST_LOG_SEV(gLogger, bltriv::trace) << "separator=' '";
                        separator = kSpace;
                    }
                }
            }
        }

        {
            // Detect quote
            BOOST_LOG_NAMED_SCOPE("Detect quote");
            if(line.front() == kDoubleQuote || line.back() == kDoubleQuote) {
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "quote=" << kDoubleQuote;
                quote = kDoubleQuote;
            } else if(line.front() == kSingleQuote || line.back() == kSingleQuote) {
                BOOST_LOG_SEV(gLogger, bltriv::trace) << "quote=" << kSingleQuote;
                quote = kSingleQuote;
            }

            if(separator && !quote) {
                auto match = [](const std::wstring& line, wchar_t separator, wchar_t quote) {
                    std::wstring patternLeft { std::wstring(1, separator) + std::wstring(1, quote) };
                    std::wstring patternRight { std::wstring(1, quote) + std::wstring(1, separator) };
                    return line.find(patternLeft) != std::wstring::npos &&
                        line.find(patternRight) != std::wstring::npos;
                };

                if(match(line, separator.value(), kDoubleQuote)) {
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
                    quote = kDoubleQuote;
                } else if(match(line, separator.value(), kSingleQuote)) {
                    BOOST_LOG_SEV(gLogger, bltriv::trace) << FUNCTION_FILE_LINE;
                    quote = kSingleQuote;
                }
            }
        }
    }
}
