#define BOOST_TEST_MODULE Master Test Suite

#include <boost/locale/localization_backend.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>
#include <thread>

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

BOOST_AUTO_TEST_CASE(test_case1)
{
    BOOST_TEST_MESSAGE("test_case1");
}
