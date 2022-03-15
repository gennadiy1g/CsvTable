#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <string>
#include <string_view>

#include "log.hpp"
#include "utilities.hpp"

namespace blocale = boost::locale;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

using namespace std::literals::string_literals;

void initLocalization() {
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

void initLogging() {
  // clang-format off
    logging::add_file_log(
#ifdef NDEBUG
        keywords::file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
        keywords::target_file_name = bfs::path(bfs::temp_directory_path() / "BuckwheatCsv.log"),
#else
        keywords::file_name = "trace.log",
        keywords::target_file_name = "trace.log",
#endif
    keywords::format = (expr::stream
#ifdef NDEBUG
        << expr::attr<unsigned int>("LineID") << ' '
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S ")
        << trivial::severity << ' '
#else
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f ")
        << expr::attr<logging::thread_id>("ThreadID") << ' '
        << trivial::severity << ' '
        << expr::format_named_scope("Scope", keywords::format = "%n (%F:%l)") << ' '
#endif
        << expr::message),

    keywords::auto_flush = true);
  // clang-format on
  logging::add_common_attributes();
  logging::core::get()->add_global_attribute("Scope", attrs::named_scope());

#ifdef NDEBUG
  logging::core::get()->set_filter(trivial::severity >= trivial::info);
#endif
}

std::pair<std::optional<wchar_t>, std::optional<wchar_t>> detectSeparatorAndQuote(const bfs::path &filePath) {
  BOOST_LOG_FUNCTION();
  auto &gLogger = GlobalLogger::get();

  std::optional<wchar_t> separator{std::nullopt}, quote{std::nullopt};

  std::wstring line{L""};
  {
    BOOST_LOG_NAMED_SCOPE("Read 1st line");
    bfs::wifstream fileStream(filePath);
    if (!fileStream) {
      throw std::runtime_error("Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) +
                               "\" for reading!"s);
    }

    std::getline(fileStream, line);
    BOOST_LOG_SEV(gLogger, trivial::trace) << "line=" << line;
  }

  boost::trim(line);
  if (line.length()) {
    constexpr wchar_t kTab{L'\t'}, kPipe{L'|'}, kSemicolon{L';'}, kComma{L','}, kSpace{L' '}, kDoubleQuote{L'\"'},
        kSingleQuote{L'\''};
    {
      // Detect separator
      BOOST_LOG_NAMED_SCOPE("Detect separator");
      if (line.find(kTab) != std::wstring::npos) {
        BOOST_LOG_SEV(gLogger, trivial::trace) << "separator=\\t";
        separator = kTab;
      } else {
        bool ambiguous{false};
        for (auto const &ch : line) {
          if (ch == kPipe || ch == kSemicolon || ch == kComma) {
            if (!separator) {
              BOOST_LOG_SEV(gLogger, trivial::trace) << "separator=" << ch;
              separator = ch;
            } else {
              if (separator.value() != ch) {
                // Ambiguous situation - multiple separators found
                BOOST_LOG_SEV(gLogger, trivial::trace) << "Ambiguous! Another separator=" << ch;
                ambiguous = true;
                separator = std::nullopt;
                break;
              }
            }
          }
        }
        if (!ambiguous && !separator) {
          if (line.find(kSpace) != std::wstring::npos) {
            BOOST_LOG_SEV(gLogger, trivial::trace) << "separator=' '";
            separator = kSpace;
          }
        }
      }
    }

    {
      // Detect quote
      BOOST_LOG_NAMED_SCOPE("Detect quote");
      if (line.front() == kDoubleQuote || line.back() == kDoubleQuote) {
        BOOST_LOG_SEV(gLogger, trivial::trace) << "quote=" << kDoubleQuote;
        quote = kDoubleQuote;
      } else if (line.front() == kSingleQuote || line.back() == kSingleQuote) {
        BOOST_LOG_SEV(gLogger, trivial::trace) << "quote=" << kSingleQuote;
        quote = kSingleQuote;
      }

      if (separator && !quote) {
        auto match = [](const std::wstring &line, wchar_t separator, wchar_t quote) {
          std::wstring patternLeft{std::wstring(1, separator) + std::wstring(1, quote)};
          std::wstring patternRight{std::wstring(1, quote) + std::wstring(1, separator)};
          return line.find(patternLeft) != std::wstring::npos && line.find(patternRight) != std::wstring::npos;
        };

        if (match(line, separator.value(), kDoubleQuote)) {
          BOOST_LOG_SEV(gLogger, trivial::trace) << "quote=" << kDoubleQuote;
          quote = kDoubleQuote;
        } else if (match(line, separator.value(), kSingleQuote)) {
          BOOST_LOG_SEV(gLogger, trivial::trace) << "quote=" << kSingleQuote;
          quote = kSingleQuote;
        }
      }
    }
  }

  return {separator, quote};
}
