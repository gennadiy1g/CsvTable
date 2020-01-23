#define BOOST_TEST_MODULE Master Test Suite

#include <boost/algorithm/string.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>

#include "CsvTable.h"
#include "log.h"
#include "utilities.h"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

struct GlobalFixture {
    GlobalFixture() {}

    ~GlobalFixture() {}

    void setup()
    {
        initLocalization();

        initLogging();

        auto& gLogger = GlobalLogger::get();
        BOOST_LOG_SEV(gLogger, bltrivial::trace) << "->" << FUNCTION_FILE_LINE;

        auto backends = blocale::localization_backend_manager::global().get_all_backends();
        std::string backendsList = std::accumulate(backends.cbegin(), backends.cend(), ""s,
            [](const std::string& a, const std::string& b) { return a + (a == "" ? "" : ", ") + b; });
        BOOST_LOG_SEV(gLogger, bltrivial::debug) << "Localization backends: " << backendsList << '.';
    }

    void teardown() {}
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_SUITE(FileLines_tests);

BOOST_AUTO_TEST_CASE(non_existing_file)
{
    BOOST_REQUIRE_THROW(FileLines(LR"^(non_existing_file)^"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_case_ZX0training_UTF_8_csv)
{
    FileLines fileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\ZX0training_UTF-8.csv)^");
    BOOST_TEST(fileLines.numLines() == 7438);
    BOOST_CHECK(boost::starts_with(fileLines.getLine(0), L"customer Id2,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(1), L"499962071,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(2), L"499946553,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(3), L"499942149,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7437), L"408339518,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7436), L"408339882,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7435), L"408339961,"));
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv)
{
    FileLines fileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\russian_UTF-8_2.csv)^");
    BOOST_TEST(fileLines.numLines() == 11);
    BOOST_CHECK(boost::starts_with(fileLines.getLine(0), L"идентификатор,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(1), L"строка1,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(2), L"строка2,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(3), L"строка3,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(10), L"строка10,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(9), L"строка9,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(8), L"строка8,"));
}

BOOST_AUTO_TEST_CASE(Hits_csv)
{
    FileLines fileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\Hits.csv)^");
    BOOST_TEST(fileLines.numLines() == 38044);
    BOOST_CHECK(boost::starts_with(fileLines.getLine(0), L"enrolid,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(0), L"enrolid,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(1), L"14702501,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(1), L"14702501,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(2), L"141188302,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(2), L"141188302,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(3), L"139747401,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(3), L"139747401,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(4), L"590641701,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(4), L"590641701,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(5), L"30347501,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(5), L"30347501,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38043), L"29392848001,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38043), L"29392848001,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38042), L"28781773901,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38042), L"28781773901,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38041), L"29391061301,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38041), L"29391061301,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38040), L"29422758401,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38040), L"29422758401,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38039), L"1672082702,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(38039), L"1672082702,"));
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(TokenizedFileLines_tests);

BOOST_AUTO_TEST_CASE(Hits_csv)
{
    TokenizedFileLines tokenizedFileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\Hits.csv)^");
    BOOST_TEST(tokenizedFileLines.numLines() == 38044);
    BOOST_TEST(tokenizedFileLines.numColumns() == 7);

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(0) == L"enrolid");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(1) == L"TKR1yr_predicted");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(2) == L"model_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(3) == L"scenario_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(4) == L"probability");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(5) == L"TKR1yr_real");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(6) == L"Hit");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(0) == L"14702501");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(2) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(3) == L"261");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(4) == L"50.0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(5) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(6) == L"1");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(0) == L"29392848001");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(3) == L"2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(4) == L"12.91");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(5) == L"0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(6) == L"0");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(0) == L"enrolid");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(1) == L"TKR1yr_predicted");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(2) == L"model_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(3) == L"scenario_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(4) == L"probability");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(5) == L"TKR1yr_real");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(6) == L"Hit");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(0) == L"14702501");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(2) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(3) == L"261");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(4) == L"50.0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(5) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(6) == L"1");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(0) == L"29392848001");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(3) == L"2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(4) == L"12.91");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(5) == L"0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043).at(6) == L"0");
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv)
{
    TokenizedFileLines tokenizedFileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\russian_UTF-8_2.csv)^");
    BOOST_TEST(tokenizedFileLines.numLines() == 11);
    BOOST_TEST(tokenizedFileLines.numColumns() == 4);

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(0) == L"идентификатор");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(1) == L" переменная1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(2) == L" переменная2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(3) == L" переменная3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(0) == L"строка1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(1) == L"красный");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(2) == L"большой");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(3) == L"далеко");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(0) == L"строка10");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(1) == L"розовый");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(2) == L"не маленький");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(3) == L"близко");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(0) == L"идентификатор");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(1) == L" переменная1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(2) == L" переменная2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0).at(3) == L" переменная3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(0) == L"строка1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(1) == L"красный");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(2) == L"большой");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1).at(3) == L"далеко");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(0) == L"строка10");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(1) == L"розовый");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(2) == L"не маленький");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10).at(3) == L"близко");
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(detectSeparatorAndQuote_tests);

BOOST_AUTO_TEST_CASE(Hits_csv)
{
    std::optional<wchar_t> separator;
    std::optional<wchar_t> quote;
    detectSeparatorAndQuote(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\Hits.csv)^", separator, quote);
    BOOST_CHECK(separator && separator.value() == L',');
    BOOST_CHECK(!quote);
}

BOOST_AUTO_TEST_CASE(Hits_Semicolon_csv)
{
    std::optional<wchar_t> separator;
    std::optional<wchar_t> quote;
    detectSeparatorAndQuote(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\Hits_Semicolon.csv)^", separator, quote);
    BOOST_CHECK(separator && separator.value() == L';');
    BOOST_CHECK(!quote);
}

BOOST_AUTO_TEST_SUITE_END();
