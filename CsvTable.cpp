#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <chrono>
#include <cstddef>

#include "CsvTable.hpp"
#include "log.hpp"

namespace blocale = boost::locale;

using namespace std::literals::string_literals;

FileLines::FileLines(const bfs::path &filePath, OnProgress onProgress)
    : mFilePath(filePath), mFileStream(filePath, std::ios_base::in | std::ios_base::binary), mOnProgress(onProgress) {
  BOOST_LOG_FUNCTION();
  auto &gLogger = GlobalLogger::get();
  BOOST_LOG_SEV(gLogger, trivial::trace) << filePath;

  checkInputFile();

  if (!mFileStream.is_open()) {
    throw std::runtime_error("Unable to open file \""s + blocale::conv::utf_to_utf<char>(filePath.native()) +
                             "\" for reading!"s);
  }

  mFileSize = bfs::file_size(mFilePath);
  assert(mFileSize);

  BOOST_LOG_SEV(gLogger, trivial::trace) << "Starting FileLines::getPositionsOfSampleLines on a new thread";
  std::thread t(&FileLines::getPositionsOfSampleLines, this);
  mThread = std::move(t);
}

void FileLines::checkInputFile() const {
  auto errorMessage = [](bfs::path const &filePath) {
    return "File \""s + blocale::conv::utf_to_utf<char>(filePath.native()) + "\" ";
  };

  auto inputFileStatus = bfs::status(mFilePath);

  if (!bfs::exists(inputFileStatus)) {
    throw std::runtime_error(errorMessage(mFilePath) + "does not exist!"s);
  }

  if (!bfs::is_regular_file(inputFileStatus)) {
    throw std::runtime_error(errorMessage(mFilePath) + "is not a regular file!"s);
  }

  if (bfs::file_size(mFilePath) == 0) {
    throw std::runtime_error(errorMessage(mFilePath) + "is empty!"s);
  }
}

void FileLines::getPositionsOfSampleLines() {
  BOOST_LOG_FUNCTION();

  auto &gLogger = GlobalLogger::get();
  std::string line;
  std::size_t numLines{0};
  int percent{0};
  auto prevTimePointC = std::chrono::system_clock::now();
  auto prevTimePointP = prevTimePointC;
  decltype(mPosSampleLine) buffer;
  decltype(mFileStream) fileStream(mFilePath, std::ios_base::in | std::ios_base::binary);
  assert(fileStream.is_open());

  auto flushBuffer = [&buffer, &numLines, this]() {
    if (buffer.size()) {
      {
        const std::lock_guard<std::mutex> lock(mMutex);
        std::copy(buffer.cbegin(), buffer.cend(), std::back_inserter(mPosSampleLine));
        mNumLines = numLines;
      }
      buffer.clear();
    }
  };

  while (fileStream.good()) {
    BOOST_LOG_NAMED_SCOPE("In the loop");

    if (fileStream.tellg() >= mFileSize) {
      break; // do not store position after the last line
    }

    if (!(numLines % mLinesSamplesRatio)) { // numLines does not include headers' line yet
      buffer.push_back(fileStream.tellg());
      BOOST_LOG_SEV(gLogger, trivial::trace)
          << "numLines=" << numLines << ", buffer[" << buffer.size() - 1 << "]=" << buffer.at(buffer.size() - 1);
    }

    if (auto timePoint = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli>(timePoint - prevTimePointC).count() > 100) {
      if (mIsCancelled) {
        // Cancelled by user
        BOOST_LOG_SEV(gLogger, trivial::trace) << "Cancelled by user";
        break;
      }
      prevTimePointC = timePoint;
    }

    std::getline(fileStream, line);

    /* Read at least that many lines, excluding headers' line, before trying to evaluate the number of lines in the
     * file */
    constexpr std::size_t kMinNumLines{1'000};
    if (numLines == kMinNumLines) {
      // Evaluate number of lines in the file, excluding headers' line
      assert(mPosSampleLine.size() >= 2); // buffer must have been flushed
      assert(fileStream.tellg() - mPosSampleLine.at(1) > 0);
      // numLines does not include headers' line yet
      auto approxNumLines = numLines * (static_cast<float>(mFileSize - mPosSampleLine.at(1)) /
                                        (fileStream.tellg() - mPosSampleLine.at(1)));
      BOOST_LOG_SEV(gLogger, trivial::trace)
          << "numLines=" << numLines << ", mFileSize=" << mFileSize << ", approxNumLines=" << approxNumLines;

      // Calculate the number of lines between successive samples
      constexpr std::size_t kMaxNumSamples{10'000}; // maximum number of sample lines, excluding headers' line
      auto linesSamplesRatio = std::max(std::lround(approxNumLines / kMaxNumSamples), 1l);
      BOOST_LOG_SEV(gLogger, trivial::trace) << "numLinesBetweenSamples=" << linesSamplesRatio;

      // Keep positions only for line numbers divisible by numLinesBetweenSamples
      if (linesSamplesRatio > 1) {
        flushBuffer();

        decltype(mPosSampleLine) keep;
        for (std::size_t i = 0; i < mPosSampleLine.size(); i += linesSamplesRatio) {
          keep.push_back(mPosSampleLine[i]);
        }

        {
          const std::lock_guard<std::mutex> lock(mMutex);
          std::swap(mPosSampleLine, keep);
          mPosSampleLine.reserve(kMaxNumSamples + 1); // kMaxNumSamples data lines plus headers' line
          assert(mPosSampleLine.at(0) == 0);
          mLinesSamplesRatio = linesSamplesRatio;
          mPosBetweenSamples.reserve(linesSamplesRatio - 1);
        }
      }
    }

    ++numLines; // numLines now includes headers' line

    constexpr std::size_t kMaxBufferSize{1'000};
    if (buffer.size() == kMaxBufferSize) {
      flushBuffer();
    }

    if (mOnProgress) {
      if (auto timePoint = std::chrono::system_clock::now();
          std::chrono::duration<float, std::milli>(timePoint - prevTimePointP).count() > 500) {
        percent = static_cast<int>(std::floor(static_cast<float>(fileStream.tellg()) / mFileSize * 100));
        BOOST_LOG_SEV(gLogger, trivial::trace) << "percent=" << percent;
        flushBuffer();
        mOnProgress(numLines, percent);
        prevTimePointP = timePoint;
      }
    }

    /* Class wxGrid uses int for number of rows. See int wxGridTableBase::GetRowsCount() const and virtual int
     * wxGridTableBase::GetNumberRows() at https://docs.wxwidgets.org/3.1.3/classwx_grid_table_base.html.
     * We do not need to get positions for more lines than the maximum number of rows that wxGrid can display. */
    constexpr std::size_t kMaxInt = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (numLines == kMaxInt) {
      BOOST_LOG_SEV(gLogger, trivial::trace) << "Maximum number of rows that wxGrid can display has been reached!";
      mIsNumLinesLimitReached = true;
      break;
    }
  }
  BOOST_LOG_SEV(gLogger, trivial::trace) << "After the loop fileStream.tellg()=" << fileStream.tellg();

  if (!fileStream.eof() && fileStream.fail()) {
    std::stringstream message;

    if (fileStream.bad()) {
      message << "Irrecoverable input error (badbit)!";
    } else {
      message << "Logical/Extraction error (failbit)!";
    }

    message << " File: \"" << blocale::conv::utf_to_utf<char>(mFilePath.native()) << "\", line: " << numLines + 1
            << ", column: " << boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line)).length() + 1 << '.';
    throw std::runtime_error(message.str());
  }

  flushBuffer();
  if (mOnProgress) {
    mOnProgress(numLines, 100);
  }
}

#define LOG_LINE_AND_POS                                                                                               \
  BOOST_LOG_SEV(gLogger, trivial::trace) << "line.substr()="                                                           \
                                         << (blocale::conv::utf_to_utf<wchar_t>(line)).substr(0, 50)                   \
                                         << ", mFileStream.tellg()=" << mFileStream.tellg()
#define LOG_POS_BETWEEN_SAMPLES                                                                                        \
  BOOST_LOG_SEV(gLogger, trivial::trace) << "mPosBetweenSamples[" << mPosBetweenSamples.size() - 1                     \
                                         << "]=" << mPosBetweenSamples.back()

std::wstring FileLines::getLine(std::size_t lineNum) {
  BOOST_LOG_FUNCTION();
  std::string line;
  const std::lock_guard<std::mutex> lock(mMutex);

  auto &gLogger = GlobalLogger::get();
  if (mLinesSamplesRatio == 1) {
    BOOST_LOG_NAMED_SCOPE("mLinesSamplesRatio == 1");
    assert(lineNum < mPosSampleLine.size());
    auto pos = mPosSampleLine.at(lineNum);
    BOOST_LOG_SEV(gLogger, trivial::trace) << "lineNum=" << lineNum << ", pos=" << pos;
    mFileStream.seekg(pos);
    std::getline(mFileStream, line);
    LOG_LINE_AND_POS;
  } else {
    BOOST_LOG_NAMED_SCOPE("mLinesSamplesRatio != 1");
    auto sampleNum = lineNum / mLinesSamplesRatio; // line number of the nearest sample
    auto rem = lineNum % mLinesSamplesRatio;
    BOOST_LOG_SEV(gLogger, trivial::trace) << "lineNum=" << lineNum << ", sampleNum=" << sampleNum << ", rem=" << rem;
    assert(sampleNum < mPosSampleLine.size());

    if (sampleNum != mPrevSampleNum) {
      BOOST_LOG_NAMED_SCOPE("mPrevSampleNum != sampleNum");
      BOOST_LOG_SEV(gLogger, trivial::trace) << "mPosBetweenSamples.clear()";
      mPosBetweenSamples.clear();
      mPrevSampleNum = sampleNum;
    }

    auto morePosBetweenSamples = [this]() {
      return (mPosBetweenSamples.size() < (mLinesSamplesRatio - 1)) && (mFileStream.tellg() < mFileSize);
    };

    BOOST_LOG_SEV(gLogger, trivial::trace) << "mPosBetweenSamples.size()=" << mPosBetweenSamples.size();

    if (!rem) {
      BOOST_LOG_NAMED_SCOPE("!rem")
      auto pos = mPosSampleLine.at(sampleNum);
      BOOST_LOG_SEV(gLogger, trivial::trace) << "pos=" << pos;
      mFileStream.seekg(pos);
      std::getline(mFileStream, line);
      LOG_LINE_AND_POS;
      if (!mPosBetweenSamples.size() && morePosBetweenSamples()) {
        mPosBetweenSamples.push_back(mFileStream.tellg());
        LOG_POS_BETWEEN_SAMPLES;
      }
    } else {
      BOOST_LOG_NAMED_SCOPE("rem")
      if (!mPosBetweenSamples.size()) {
        BOOST_LOG_NAMED_SCOPE("!mPosBetweenSamples.size()");
        auto pos = mPosSampleLine.at(sampleNum);
        BOOST_LOG_SEV(gLogger, trivial::trace) << "pos=" << pos;
        mFileStream.seekg(pos);
        std::getline(mFileStream, line);
        LOG_LINE_AND_POS;
        if (morePosBetweenSamples()) {
          mPosBetweenSamples.push_back(mFileStream.tellg());
          LOG_POS_BETWEEN_SAMPLES;
        }

        for (std::size_t i = 0; i < rem; ++i) {
          std::getline(mFileStream, line);
          LOG_LINE_AND_POS;
          if (morePosBetweenSamples()) {
            mPosBetweenSamples.push_back(mFileStream.tellg());
            LOG_POS_BETWEEN_SAMPLES;
          }
        }
      } else {
        BOOST_LOG_NAMED_SCOPE("mPosBetweenSamples.size()");
        assert(mPosBetweenSamples.size() <= mLinesSamplesRatio - 1);
        if (rem <= mPosBetweenSamples.size()) {
          BOOST_LOG_NAMED_SCOPE("rem <= mPosBetweenSamples.size()")
          auto pos = mPosBetweenSamples.at(rem - 1);
          BOOST_LOG_SEV(gLogger, trivial::trace) << "pos=" << pos;
          mFileStream.seekg(pos);
          std::getline(mFileStream, line);
          LOG_LINE_AND_POS;
          if ((rem == mPosBetweenSamples.size()) && morePosBetweenSamples()) {
            mPosBetweenSamples.push_back(mFileStream.tellg());
            LOG_POS_BETWEEN_SAMPLES;
          }
        } else {
          BOOST_LOG_NAMED_SCOPE("rem > mPosBetweenSamples.size()")
          auto pos = mPosBetweenSamples.back();
          BOOST_LOG_SEV(gLogger, trivial::trace) << "pos=" << pos;
          mFileStream.seekg(pos);
          /* The last pos in mPosBetweenSamples is for the line that has not been read yet, hence plus one.
           * Do not eliminate varible reps by putting the expression directly into the loop's condition,
           * because size of mPosBetweenSamples changes in the loop's body. */
          auto reps = rem - mPosBetweenSamples.size() + 1;
          for (std::size_t i = 0; i < reps; ++i) {
            std::getline(mFileStream, line);
            LOG_LINE_AND_POS;
            if (morePosBetweenSamples()) {
              mPosBetweenSamples.push_back(mFileStream.tellg()); // changes size of mPosBetweenSamples!
              LOG_POS_BETWEEN_SAMPLES;
            }
          }
        }
      }
    }
  }
  return boost::trim_right_copy(blocale::conv::utf_to_utf<wchar_t>(line));
}

void TokenizedFileLines::setTokenFuncParams(wchar_t escape, wchar_t separator, wchar_t quote) {
  BOOST_LOG_FUNCTION();
  auto &gLogger = GlobalLogger::get();

  if (separator != mSeparator || quote != mQuote || escape != mEscape) {
    BOOST_LOG_SEV(gLogger, trivial::trace) << "mTokenizedLines.clear()";
    mTokenizedLines.clear();
  }

  mEscapedListSeparator = EscapedListSeparator(escape, separator, quote);
  mEscape = escape;
  mSeparator = separator;
  mQuote = quote;
}

const std::vector<std::wstring> *TokenizedFileLines::getTokenizedLine(std::size_t lineNum) {
  BOOST_LOG_FUNCTION();
  auto &gLogger = GlobalLogger::get();
  BOOST_LOG_SEV(gLogger, trivial::trace) << "lineNum=" << lineNum;

  if (auto search = mTokenizedLines.find(lineNum); search != mTokenizedLines.end()) {
    BOOST_LOG_NAMED_SCOPE("search != mTokenizedLines.end()");
    return &search->second;
  } else {
    BOOST_LOG_NAMED_SCOPE("search == mTokenizedLines.end()");
    if (mTokenizedLines.size() == kMaxSize) {
      /* The size of the map is at the maximum. Remove one element from the map - the element
       * that is furthest away from lineNum. */
      BOOST_LOG_NAMED_SCOPE("mTokenizedLines.size() == kMaxSize");
      // The logic below works only if there are more than 4 elements in the map
      assert(mTokenizedLines.size() > 4);
      auto itFirst = mTokenizedLines.cbegin();
      if (itFirst->first == 0) {
        // Always keep the line #0 because it contains columns' names
        ++itFirst;
      }
      auto itLast = mTokenizedLines.crbegin();

      BOOST_LOG_SEV(gLogger, trivial::trace)
          << "itFirst->first=" << itFirst->first << ", itLast->first=" << itLast->first;

      std::size_t distToFirst = lineNum >= itFirst->first ? lineNum - itFirst->first : itFirst->first - lineNum;
      std::size_t distToLast = itLast->first >= lineNum ? itLast->first - lineNum : lineNum - itLast->first;
      if (distToFirst >= distToLast) {
        BOOST_LOG_SEV(gLogger, trivial::trace) << "Erasing itFirst line #" << itFirst->first;
        mTokenizedLines.erase(itFirst);
      } else {
        BOOST_LOG_SEV(gLogger, trivial::trace) << "Erasing itLast->first line #" << itLast->first;
        mTokenizedLines.erase(itLast->first);
      }
    }

    auto line = mFileLines.getLine(lineNum);
    BOOST_LOG_SEV(gLogger, trivial::trace) << "line.substr()=" << line.substr(0, 50);
    LineTokenizer tok(line, mEscapedListSeparator);
    std::vector<std::wstring> tokenizedLine;
    for (auto beg = tok.begin(); beg != tok.end(); ++beg) {
      tokenizedLine.push_back(*beg);
    }
    const auto [it, success] = mTokenizedLines.insert({lineNum, std::move(tokenizedLine)});
    assert(success);
    BOOST_LOG_SEV(gLogger, trivial::trace) << "Inserted line #" << lineNum;
    return &it->second;
  }
}
