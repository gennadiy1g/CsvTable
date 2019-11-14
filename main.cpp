#define BOOST_TEST_MODULE Master Test Suite

#include <boost/algorithm/string.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>
#include <thread>

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

        BOOST_LOG_SEV(gLogger, bltrivial::info) << std::thread::hardware_concurrency() << " concurrent threads are supported.";
    }

    void teardown() {}
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_CASE(non_existing_file)
{
    BOOST_REQUIRE_THROW(FileLines(LR"^(non_existing_file)^"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_case_ZX0training_UTF_8_csv)
{
    FileLines fileLines(LR"^(C:\Users\genna_000\Documents\BuckwheatCsv\test data\ZX0training_UTF-8.csv)^");
    BOOST_TEST(fileLines.numLines() == 0);
    fileLines.getPositionsOfSampleLines();
    BOOST_TEST(fileLines.numLines() == 7438);
    BOOST_CHECK(boost::starts_with(fileLines.getLine(0), L"499962071,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(1), L"499946553,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(2), L"499942149,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7436), L"408339518,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7435), L"408339882,"));
    BOOST_CHECK(boost::starts_with(fileLines.getLine(7434), L"408339961,"));
}
