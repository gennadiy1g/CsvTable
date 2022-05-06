#define BOOST_TEST_MODULE Master Test Suite

#include <boost/algorithm/string.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>

#include "CsvTable.hpp"
#include "SepChars.hpp"
#include "log.hpp"
#include "utilities.hpp"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

#if defined(__unix__)
const auto kTestDataDir = LR"^(/home/gennadiy/Projects/BuckwheatCsv2/test data/)^"s;
#else
const auto kTestDataDir = LR"^(C:\Users\genna_000\Documents\BuckwheatCsv2\test data\)^"s;
#endif

struct GlobalFixture {
  GlobalFixture() {}

  ~GlobalFixture() {}

  void setup() {
    BOOST_LOG_FUNCTION();

    initLocalization();
    initLogging();

    auto backends = blocale::localization_backend_manager::global().get_all_backends();
    std::string backendsList =
        std::accumulate(backends.cbegin(), backends.cend(), ""s,
                        [](const std::string &a, const std::string &b) { return a + (a == "" ? "" : ", ") + b; });
    auto &gLogger = GlobalLogger::get();
    BOOST_LOG_SEV(gLogger, trivial::debug) << " Localization backends: " << backendsList << '.';
  }

  void teardown() {}
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);

BOOST_AUTO_TEST_SUITE(FileLines_tests);

BOOST_AUTO_TEST_CASE(non_existing_file) {
  BOOST_REQUIRE_THROW(FileLines(LR"^(non_existing_file)^"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_case_ZX0training_UTF_8_csv) {
  FileLines fileLines(kTestDataDir + LR"^(ZX0training_UTF-8.csv)^"s);
  fileLines.joinWorkerThread();
  BOOST_TEST(fileLines.numLines() == 7438);
  BOOST_TEST(boost::starts_with(fileLines.getLine(0), L"customer Id2,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(1), L"499962071,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(2), L"499946553,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(3), L"499942149,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(7437), L"408339518,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(7436), L"408339882,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(7435), L"408339961,"));
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv) {
  FileLines fileLines(kTestDataDir + LR"^(russian_UTF-8_2.csv)^");
  fileLines.joinWorkerThread();
  BOOST_TEST(fileLines.numLines() == 11);
  BOOST_TEST(boost::starts_with(fileLines.getLine(0), L"идентификатор,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(1), L"строка1,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(2), L"строка2,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(3), L"строка3,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(10), L"строка10,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(9), L"строка9,"));
  BOOST_TEST(boost::starts_with(fileLines.getLine(8), L"строка8,"));
}

BOOST_AUTO_TEST_CASE(Hits_csv) {
  FileLines fileLines(kTestDataDir + LR"^(Hits.csv)^");
  fileLines.joinWorkerThread();
  BOOST_TEST(fileLines.numLines() == 38044);
  for (auto i = 0; i < 2; ++i) {
    BOOST_TEST(boost::starts_with(fileLines.getLine(0), L"enrolid,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1), L"14702501,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(2), L"141188302,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(3), L"139747401,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(4), L"590641701,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(5), L"30347501,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(38043), L"29392848001,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(38042), L"28781773901,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(38041), L"29391061301,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(38040), L"29422758401,"));
    BOOST_TEST(boost::starts_with(fileLines.getLine(38039), L"1672082702,"));
  }
}

BOOST_AUTO_TEST_CASE(web_complex_data_with_target_variable_csv) {
  FileLines fileLines(kTestDataDir + LR"^(web complex data with target variable.csv)^");
  fileLines.joinWorkerThread();
  BOOST_TEST(fileLines.numLines() == 1035808);
  for (auto i = 0; i < 2; ++i) {
    BOOST_TEST(boost::starts_with(fileLines.getLine(0), L"id,parent_id,cluster,program_id,offer_id,affiliate_id,"));
    BOOST_TEST(boost::starts_with(
        fileLines.getLine(1), L"328090022,\\N,22,1,9656,43608,firstsub,secondsub,496940,\\N,53479,11,Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(2),
                                  L"328375080,\\N,22,1,9656,43608,firstsub,\\N,496940,\\N,53479,11,Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(3),
                                  L"328436381,\\N,22,1,9656,43608,zone10059,\\N,496940,\\N,53479,11,Mozilla/4.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(4),
                                  L"328588235,\\N,22,1,9656,43608,zone10059,\\N,496940,\\N,53479,11,Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(5),
                                  L"328636022,\\N,22,1,9656,43608,zone10059,\\N,496940,\\N,53479,11,Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1035807),
                                  L"934804528,\\N,12,1,9656,43608,zone10061,\\N,496944,\\N,53479,11,Mozilla/4.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1035806),
                                  L"934802516,\\N,12,1,9656,43608,zone10061,\\N,496944,\\N,53479,11,Mozilla/4.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1035805),
                                  L"934802243,\\N,12,1,9656,43608,zone10061,\\N,496940,\\N,53479,11,\"Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1035804),
                                  L"934801910,\\N,12,1,9656,43608,zone10061,\\N,496940,\\N,53479,11,Mozilla/5.0 "));
    BOOST_TEST(boost::starts_with(fileLines.getLine(1035803),
                                  L"934801729,\\N,12,1,9656,43608,zone10061,\\N,496940,\\N,53479,11,Mozilla/5.0 "));
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(TokenizedFileLines_tests);

BOOST_AUTO_TEST_CASE(Hits_csv) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(Hits.csv)^");
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 38044);
  BOOST_TEST(tokenizedFileLines.numColumns() == 7);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"enrolid");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(1) == L"TKR1yr_predicted");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(2) == L"model_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(3) == L"scenario_number");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(4) == L"probability");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(5) == L"TKR1yr_real");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(6) == L"Hit");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"14702501");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(2) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(3) == L"261");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(4) == L"50.0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(5) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(6) == L"1");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(0) == L"29392848001");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(1) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(3) == L"2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(4) == L"12.91");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(5) == L"0");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(38043)->at(6) == L"0");
  }
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(russian_UTF-8_2.csv)^");
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 11);
  BOOST_TEST(tokenizedFileLines.numColumns() == 4);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"идентификатор");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(1) == L"переменная1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(2) == L"переменная2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(3) == L"переменная3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"строка1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(1) == L"красный");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(2) == L"большой");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(3) == L"далеко");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(0) == L"строка10");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(1) == L"розовый");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(2) == L"не маленький");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(3) == L"близко");
  }
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv_mod) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(russian_UTF-8_2.csv)^");
  tokenizedFileLines.setTokenFuncParams(kNull, kTab, kDoubleQuote);
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 11);
  BOOST_TEST(tokenizedFileLines.numColumns() == 1);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"идентификатор,переменная1,переменная2,переменная3");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"строка1,красный,большой,далеко");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(0) == L"строка10,розовый,не маленький,близко");
  }
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_csv_mod_wrong_esc) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(russian_UTF-8_2.csv)^");
  tokenizedFileLines.setTokenFuncParams(L'1', kTab, kDoubleQuote);
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 11);
  BOOST_TEST(tokenizedFileLines.numColumns() == 1);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"идентификатор,переменная1,переменная2,переменная3");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"строка1,красный,большой,далеко");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(0) == L"строка10,розовый,не маленький,близко");
  }
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_Tab_SingleQuote) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(russian_UTF-8_2_Tab_SingleQuote.csv)^");
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 11);
  BOOST_TEST(tokenizedFileLines.numColumns() == 1);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) ==
                L"'идентификатор'	'переменная1'	'переменная2'	'переменная3'");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) ==
                L"'строка1'	'красный'	'большой'	'далеко'");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(0) ==
                L"'строка10'	'розовый'	'не маленький'	'близко'");
  }

  tokenizedFileLines.setTokenFuncParams(kNull, kTab, kSingleQuote);
  BOOST_TEST(tokenizedFileLines.numLines() == 11);
  BOOST_TEST(tokenizedFileLines.numColumns() == 4);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"идентификатор");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(1) == L"переменная1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(2) == L"переменная2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(3) == L"переменная3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"строка1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(1) == L"красный");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(2) == L"большой");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(3) == L"далеко");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(0) == L"строка10");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(1) == L"розовый");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(2) == L"не маленький");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(10)->at(3) == L"близко");
  }
}

BOOST_AUTO_TEST_CASE(web_complex_data_with_target_variable_csv) {
  TokenizedFileLines tokenizedFileLines(kTestDataDir + LR"^(web complex data with target variable.csv)^");
  tokenizedFileLines.joinWorkerThread();
  BOOST_TEST(tokenizedFileLines.numLines() == 1035808);
  BOOST_TEST(tokenizedFileLines.numColumns() == 65);

  for (auto i = 0; i < 2; ++i) {
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(0) == L"id");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(1) == L"parent_id");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(2) == L"cluster");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(3) == L"program_id");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(4) == L"offer_id");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(5) == L"affiliate_id");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(6) == L"sub");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(12) == L"browser");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(13) == L"ip");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(14) == L"referer");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(15) == L"referer2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(62) == L"minute");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(63) == L"second");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(0)->at(64) == L"WEEK_DAY");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(0) == L"328090022");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(2) == L"22");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(6) == L"firstsub");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(12) ==
                L"Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.1.5) Gecko/20091102 Firefox/3.5.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(13) == L"67.226.150.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(14) == L"http://www.scottbrooks.ca/epic/");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(15) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(62) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(63) == L"23");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1)->at(64) == L"3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(0) == L"328375080");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(2) == L"22");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(6) == L"firstsub");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(12) ==
                L"Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.1.5) Gecko/20091102 Firefox/3.5.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(13) == L"67.226.150.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(14) == L"http://www.scottbrooks.ca/epic/");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(15) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(62) == L"48");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(63) == L"47");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(2)->at(64) == L"3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(0) == L"328436381");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(2) == L"22");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(6) == L"zone10059");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(12) ==
                L"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; GTB6.3; .NET CLR 1.1.4322)");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(13) == L"72.39.10.16");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(14) == L"http://www5.azoogleads.com/rocky.html");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(15) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(62) == L"59");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(63) == L"55");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(3)->at(64) == L"3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(0) == L"328588235");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(2) == L"22");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(6) == L"zone10059");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(12) ==
                L"Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.1.5) Gecko/20091102 Firefox/3.5.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(13) == L"67.226.150.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(14) == L"http://www5.azoogleads.com/rocky.html");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(15) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(62) == L"17");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(63) == L"54");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(4)->at(64) == L"3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(0) == L"328636022");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(2) == L"22");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(6) == L"zone10059");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(12) ==
                L"Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.1.5) Gecko/20091102 Firefox/3.5.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(13) == L"67.226.150.5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(14) == L"http://www5.azoogleads.com/rocky.html");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(15) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(62) == L"18");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(63) == L"43");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(5)->at(64) == L"3");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(0) == L"934804528");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(6) == L"zone10061");
    BOOST_CHECK(
        tokenizedFileLines.getTokenizedLine(1035807)->at(12) ==
        L"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322; .NET CLR "
        L"2.0.50727; .NET "
        L"CLR 3.0.4506.2152; .NET CLR 3.5.30729; InfoPath.2; MS-RTC LM 8)");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(13) == L"192.206.119.3");
    BOOST_CHECK(
        tokenizedFileLines.getTokenizedLine(1035807)->at(14) ==
        L"http://ad.yieldmanager.com/"
        L"iframe3?"
        L"01QAADxWDABeQkIAAAAAAPiTEQAAAAAAAgAEAAIAAAAAAP8AAAACCRHDGAAAAAAAIKAIAAAAAACEURgAAAAAAAAAAAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAABFEAMAAAAAAAIAAwAAAAAAmpmZmZmZ4T-amZmZmZnhPwAAAAAAAOw.AAAAAAAA7D8AAAAAAAD0PwAAAAAA");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(15) ==
                L"http://www.sendspace.com/defaults/framer.html?zone=1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(62) == L"31");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(63) == L"5");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035807)->at(64) == L"2");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(0) == L"934802516");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(6) == L"zone10061");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(12) ==
                L"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; .NET CLR 2.0.50727; .NET CLR "
                L"3.0.4506.2152; .NET CLR "
                L"3.5.30729)");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(13) == L"24.220.244.242");
    BOOST_CHECK(
        tokenizedFileLines.getTokenizedLine(1035806)->at(14) ==
        L"http://ad.yieldmanager.com/"
        L"iframe3?"
        L"01QAADxWDABeQkIAAAAAAPiTEQAAAAAAAgAAAAIAAAAAAP8AAAACCRHDGAAAAAAAIKAIAAAAAACEURgAAAAAAAAAAAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAABFEAMAAAAAAAIAAwAAAAAAmpmZmZmZ4T-amZmZmZnhPwAAAAAAAOw.AAAAAAAA7D8AAAAAAAD0PwAAAAAA");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(15) ==
                L"http://www.sendspace.com/defaults/framer.html?zone=1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(62) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(63) == L"20");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035806)->at(64) == L"2");

    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(0) == L"934802243");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(1) == L"\\N");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(2) == L"12");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(3) == L"1");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(4) == L"9656");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(5) == L"43608");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(6) == L"zone10061");
    BOOST_CHECK(
        tokenizedFileLines.getTokenizedLine(1035805)->at(12) ==
        L"Mozilla/5.0 (Linux; U; Android 1.1; en-us; dream) AppleWebKit/525.10+ (KHTML, like Gecko) Version/3.0.4 "
        L"Mobile "
        L"Safari/523.12.2");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(13) == L"208.54.87.49");
    BOOST_CHECK(
        tokenizedFileLines.getTokenizedLine(1035805)->at(14) ==
        L"http://ad.yieldmanager.com/"
        L"iframe3?"
        L"p4tAANm8CwBeQkIAAAAAAPiTEQAAAAAAAgAAAAIAAAAAAP8AAAACCreyFQAAAAAAWt8VAAAAAACEURgAAAAAAAAAAAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAAA"
        L"AAAAAAAAAAAAAAAAACEgYAAAAAAAIAAwAAAAAAmpmZmZmZ4z-amZmZmZnjPwAAAAAAAOw.AAAAAAAA7D8AAAAAAAD0PwAAAAAA");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(15) ==
                L"http://ad.adperium.com/st?ad_type=iframe&ad_size=300x250&section=769241");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(62) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(63) == L"4");
    BOOST_CHECK(tokenizedFileLines.getTokenizedLine(1035805)->at(64) == L"2");
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(detectSeparatorAndQuote_tests);

BOOST_AUTO_TEST_CASE(Hits_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(Hits.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kComma));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(Hits_Semicolon_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(Hits_Semicolon.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kSemicolon));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(Hits_Space_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(Hits_Space.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kSpace));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(Hits_Tab_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(Hits_TAB.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kTab));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(Hits_VerticalBar_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(Hits_VerticalBar.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kPipe));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_DoubleQuote_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(russian_UTF-8_2_DoubleQuote.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kComma));
  BOOST_TEST((quote && quote.value() == kDoubleQuote));
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_SingleQuote_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(russian_UTF-8_2_SingleQuote.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kComma));
  BOOST_TEST((quote && quote.value() == kSingleQuote));
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_Tab_SingleQuote_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(russian_UTF-8_2_Tab_SingleQuote.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kTab));
  BOOST_TEST((quote && quote.value() == kSingleQuote));
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_Tab_SingleQuote_2_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(russian_UTF-8_2_Tab_SingleQuote_2.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kTab));
  BOOST_TEST((quote && quote.value() == kSingleQuote));
}

BOOST_AUTO_TEST_CASE(file_empty_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(file_empty.csv)^", separator, quote);
  BOOST_TEST((separator && separator.value() == kComma));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(russian_UTF_8_2_Ambiguous_csv) {
  std::optional<wchar_t> separator, quote;
  detectSeparatorAndQuote(kTestDataDir + LR"^(russian_UTF-8_2_Ambiguous.csv)^", separator, quote);
  BOOST_TEST((!separator));
  BOOST_TEST(!quote);
}

BOOST_AUTO_TEST_CASE(not_text_file) {
  std::optional<wchar_t> separator, quote;
  BOOST_REQUIRE_THROW(detectSeparatorAndQuote(kTestDataDir + LR"^(OpenDialog.png)^", separator, quote),
                      std::ios_base::failure);
}

BOOST_AUTO_TEST_SUITE_END();
