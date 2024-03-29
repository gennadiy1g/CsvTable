#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "SepChars.hpp"

namespace bfs = boost::filesystem;

class FileLines {
public:
  using OnProgress = std::function<void(std::size_t, int)>;

  explicit FileLines(const bfs::path &filePath, OnProgress onProgress = OnProgress()); // Constructor
  virtual ~FileLines();

  // Disallow assignment and pass-by-value.
  FileLines(const FileLines &src) = delete;
  FileLines &operator=(const FileLines &rhs) = delete;

  /* No move operations because field mFileStream has inaccessible move constructor and
   * move assignement operator */
  FileLines(FileLines &&src) = delete;
  FileLines &operator=(FileLines &&rhs) = delete;

  std::size_t numLines() const { return mNumLines; };

  bool isNumLinesLimitReached() const { return mIsNumLinesLimitReached; }

  std::wstring getLine(std::size_t lineNum);

  void stopReading();

  void joinWorkerThread();

private:
  void checkInputFile() const;
  void getPositionsOfSampleLines();

  bfs::path mFilePath;
  bfs::ifstream mFileStream;
  OnProgress mOnProgress;
  std::size_t mPrevSampleNum{std::numeric_limits<std::size_t>::max()};

  // Shared between this class and GUI
  std::atomic_size_t mNumLines{}; // Number of lines in the file
  std::atomic_bool mStopRequested{false};
  std::atomic_bool mIsNumLinesLimitReached{false};

  // Shared between getPositionsOfSampleLines() and getLine(); read only by both.
  bfs::ifstream::pos_type mFileSize{};

  // Shared between getPositionsOfSampleLines() and getLine(); written by at least one.
  std::vector<bfs::ifstream::pos_type> mPosSampleLine; // positions of sample lines
  std::size_t mLinesSamplesRatio{1};                   // lines to samples ratio
  decltype(mPosSampleLine) mPosBetweenSamples;         // positions of lines between sample lines

  std::mutex mMutex;
  std::thread mThread;
};

using EscapedListSeparator = boost::escaped_list_separator<wchar_t, std::char_traits<wchar_t>>;
using LineTokenizer = boost::tokenizer<EscapedListSeparator, std::wstring::const_iterator, std::wstring>;

class TokenizedFileLines {
public:
  explicit TokenizedFileLines(const bfs::path &filePath, FileLines::OnProgress onProgress = FileLines::OnProgress())
      : mFileLines(filePath, onProgress){}; // Constructor
  virtual ~TokenizedFileLines() = default;  // Defaulted virtual destructor

  // Disallow assignment and pass-by-value.
  TokenizedFileLines(const TokenizedFileLines &src) = delete;
  TokenizedFileLines &operator=(const TokenizedFileLines &rhs) = delete;

  /* No move operations because field mFileLines has deleted move constructor and
   * move assignement operator */
  TokenizedFileLines(TokenizedFileLines &&src) = delete;
  TokenizedFileLines &operator=(TokenizedFileLines &&rhs) = delete;

  // Set TokenizerFunction's paremeters
  void setTokenFuncParams(wchar_t escape, wchar_t separator, wchar_t quote);

  std::size_t numLines() const { return mFileLines.numLines(); };

  bool isNumLinesLimitReached() const { return mFileLines.isNumLinesLimitReached(); }

  std::size_t numColumns() { return getTokenizedLine(0)->size(); };

  const std::vector<std::wstring> *getTokenizedLine(std::size_t lineNum);

  void stopReading() { mFileLines.stopReading(); };

  void joinWorkerThread() { mFileLines.joinWorkerThread(); }

private:
  FileLines mFileLines;
  EscapedListSeparator mEscapedListSeparator{kNull, kComma, kDoubleQuote};
  EscapedListSeparator mNoEscEscapedListSeparator{mEscapedListSeparator};
  std::map<std::size_t, std::vector<std::wstring>> mTokenizedLines;
  static constexpr std::size_t kMaxSize{10'000};
  wchar_t mSeparator{kComma};
  wchar_t mEscape{kNull};
  wchar_t mQuote{kDoubleQuote};
};
