/*
 * logging.h
 *
 *  Created on: 16.04.2011
 *      Author: daniel
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <list>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;
#include <regen/config.h>

// Defeat evil windows defines...
#ifdef ERROR
#undef ERROR
#endif
#ifdef FATAL
#undef FATAL
#endif
#ifdef WARN
#undef WARN
#endif
#ifdef INFO
#undef INFO
#endif

namespace regen {
/**
 * Log a message using the logging framework.
 * Note: you can provide data separated by '<<' as second argument.
 */
#define LOG_MESSAGE(level, ...) {\
  stringstream ss;\
  ss << __VA_ARGS__;\
  Logging::log(level, ss.str(), __FILE__, __LINE__);\
}

/**
 * Information for the user.
 */
#define INFO_LOG(...) LOG_MESSAGE(Logging::INFO, __VA_ARGS__)
/**
 * A warning, the program still can continue without problems.
 */
#define WARN_LOG(...) LOG_MESSAGE(Logging::WARN, __VA_ARGS__)
/**
 * A error occurred, program may fail to continue.
 */
#define ERROR_LOG(...) LOG_MESSAGE(Logging::ERROR, __VA_ARGS__)
/**
 * A fatal error occurred, program might fail to continue.
 */
#define FATAL_LOG(...) LOG_MESSAGE(Logging::FATAL, __VA_ARGS__)
/**
 * Information for the developer.
 */
#define DEBUG_LOG(...) LOG_MESSAGE(Logging::DEBUG, __VA_ARGS__)

/**
 * Assert a boolean expression.
 */
#ifdef REGEN_DEBUG_BUILD
#include <cassert>
#define REGEN_ASSERT(v) assert(v)
#else
#define REGEN_ASSERT(v)
#endif

class Logger; // forward declaration

/**
 * \brief a simple logging framework.
 *
 * Can handle multiple loggers for each level defined
 * in the LogLevel enum.
 */
class Logging {
public:
  /**
   * usual logging levels.
   */
  enum LogLevel {
    INFO,
    WARN,
    ERROR,
    FATAL,
    DEBUG,
    LAST_LEVEL
  };
  /**
   * Defines different levels of verbosity.
   */
  enum Verbosity {
    _,
    V,
    VV,
    VVV
  };

  /**
   * Sets overall verbosity level.
   */
  static void set_verbosity(Verbosity verbosity);
  /**
   * Gets overall verbosity level.
   */
  static Verbosity verbosity();

  /**
   * Log a message with a specified logging level.
   */
  static void log(Logging::LogLevel level,
      const string &message, const char *file, int line);
  /**
   * Add a logger to the logging framework.
   */
  static void addLogger(Logger *logger);
  /**
   * Remove a logger from the logging framework.
   */
  static void removeLogger(Logger *logger);

private:
  static list<Logger*> loggers_[LAST_LEVEL];
  static Verbosity verbosity_;
};

/**
 * \brief Base class for loggers.
 */
class Logger
{
public:
  /**
   * Key for the actual message.
   * For formatted logging.
   */
  static const char MessageKey;
  /**
   * Key for the file the logging was triggered.
   * For formatted logging.
   */
  static const char FileKey;
  /**
   * Key for the file line the logging was triggered.
   * For formatted logging.
   */
  static const char LineKey;
  /**
   * Key for the date the logging was triggered.
   * For formatted logging.
   */
  static const char DateKey;
  /**
   * Key for the time the logging was triggered.
   * For formatted logging.
   */
  static const char TimeKey;
  /**
   * Key for the logging level.
   * For formatted logging.
   */
  static const char LevelKey;

  /**
   * Create a new logger with specified level.
   */
  Logger(Logging::LogLevel level);

  /**
   * Log a message.
   */
  void log(const string &message,
      const string file, int line);

  /**
   * Set the format for log messages.
   * You can use the key constants specified in this class.
   */
  void set_format(const string &format);

  /**
   * @return the logger level
   */
  Logging::LogLevel level() const;

  /**
   * Sets flags for stream outout.
   */
  void set_flag(ios_base::fmtflags flags);
  /**
   * Sets flags for stream outout.
   */
  void set_flag(ios_base::fmtflags flags, ios_base::fmtflags mask);
  /**
   * Sets precision of float values.
   */
  void set_precisin(streamsize precision);
  /**
   * Sets width of output.
   */
  void set_width(streamsize width);

  /**
   * Return the associated stream.
   */
  virtual ostream& stream() = 0;

protected:
  Logging::LogLevel level_;
  ios_base::fmtflags loggerFlags_, originalFlags_;
  streamsize loggerPrecision_, originalPrecision_;
  streamsize loggerWidth_, originalWidth_;

  string format_;

  void updateOS();
};

/**
 * \brief Log to a file.
 */
class FileLogger : public Logger
{
public:
  /**
   * @param level the log level.
   * @param path output file path.
   * @param mode output file open mode.
   */
  FileLogger(Logging::LogLevel level,
      const string &path, ios::openmode mode=ios::out);
  virtual ~FileLogger();
  // override
  ostream& stream();

protected:
  ofstream *file_;
};

/**
 * \brief Log to cout.
 */
class CoutLogger : public Logger {
public:
  /**
   * @param level the log level.
   */
  CoutLogger(Logging::LogLevel level);
  // override
  ostream& stream();
};

/**
 * \brief Log to cerr.
 */
class CerrLogger : public Logger {
public:
  /**
   * @param level the log level.
   */
  CerrLogger(Logging::LogLevel level);
  // override
  ostream& stream();
};

} // namespace

#endif /* LOGGING_H_ */
