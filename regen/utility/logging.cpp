#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>

#include "logging.h"

using namespace regen;

std::array<std::vector<Logger *>, Logging::LAST_LEVEL> Logging::loggers_ = {
		std::vector<Logger *>(), std::vector<Logger *>(),
		std::vector<Logger *>(), std::vector<Logger *>(), std::vector<Logger *>()
};
Logging::Verbosity Logging::verbosity_ = Logging::_;

const char Logger::DateKey = 'd';
const char Logger::FileKey = 'f';
const char Logger::LevelKey = 'v';
const char Logger::LineKey = 'l';
const char Logger::MessageKey = 'm';
const char Logger::TimeKey = 't';

void Logging::set_verbosity(Logging::Verbosity verbosity) {
	verbosity_ = verbosity;
}

Logging::Verbosity Logging::verbosity() {
	return verbosity_;
}

void Logging::addLogger(Logger *logger) {
	loggers_[logger->level()].push_back(logger);
}

void Logging::removeLogger(Logger *logger) {
	loggers_[logger->level()].erase(
		std::ranges::find(loggers_[logger->level()], logger));
}

void Logging::shutdown() {
	for (int i = 0; i < LAST_LEVEL; ++i) {
		for (auto it = loggers_[i].begin(); it != loggers_[i].end(); ++it) {
			delete *it;
		}
		loggers_[i].clear();
	}
}


void Logging::log(LogLevel level, const std::string &message, const char *filePath, int line) {
	if (message.empty()) { return; }

	std::string fileName(filePath);
	{
		size_t pos = fileName.find_last_of("/");
		if (pos != fileName.npos) {
			fileName = fileName.substr(pos + 1);
		}
	}
	for (auto it : loggers_[level]) {
		it->log(message, fileName, line);
	}
}

/////////////

Logger::Logger(Logging::LogLevel level)
		: level_(level), format_("[%t] %v: '%m' in %f(line %l)") {
}

void Logger::log(const std::string_view message, const std::string_view file, int line) {
	using namespace boost::posix_time;
	static std::locale locDate(std::cout.getloc(), new time_facet("%d.%m.%Y"));
	static std::locale locTime(std::cout.getloc(), new time_facet("%H:%M:%S"));

	std::ostream &os = stream();
	os.flags(loggerFlags_); // format flags
	os.precision(loggerPrecision_); // floating-point decimal precision
	os.width(loggerWidth_); // field width

	boost::posix_time::ptime now = second_clock::universal_time();

	for (unsigned int i = 0; i < format_.size(); ++i) {
		char &c = format_[i];
		if (c == '%') {
			i += 1;
			switch (format_[i]) {
				case '%':
					os << '%';
					break;
				case MessageKey:
					os << message;
					break;
				case FileKey:
					os << file;
					break;
				case LineKey:
					os << line;
					break;
				case DateKey: {
					std::stringstream dateStream;
					dateStream.imbue(locDate);
					dateStream << now;
					os << dateStream.str();
					break;
				}
				case TimeKey: {
					std::stringstream timeStream;
					timeStream.imbue(locTime);
					timeStream << now;
					os << timeStream.str();
					break;
				}
				case LevelKey:
					switch (level_) {
						case Logging::INFO:
							os << "INFO";
							break;
						case Logging::WARN:
							os << "WARN";
							break;
						case Logging::ERROR:
							os << "ERROR";
							break;
						case Logging::FATAL:
							os << "FATAL";
							break;
						case Logging::DEBUG:
							os << "DEBUG";
							break;
						case Logging::LAST_LEVEL:
							break;
					}
					break;
			}
		} else {
			os << c;
		}
	}
	os << std::endl;
	os.flush();

	os.flags(originalFlags_); // format flags
	os.precision(originalPrecision_); // floating-point decimal precision
	os.width(originalWidth_); // field width

}

void Logger::set_format(const std::string &format) {
	format_ = format;
}

Logging::LogLevel Logger::level() const {
	return level_;
}

void Logger::set_flag(std::ios_base::fmtflags flags) {
	loggerFlags_ |= flags;
}

void Logger::set_flag(std::ios_base::fmtflags flags, std::ios_base::fmtflags mask) {
	loggerFlags_ = ((flags & mask) | (loggerFlags_ & ~mask));
}

void Logger::set_precision(std::streamsize precision) {
	loggerPrecision_ = precision;
}

void Logger::set_width(std::streamsize width) {
	loggerWidth_ = width;
}

void Logger::updateOS() {
	std::ostream &os = stream();
	originalFlags_ = os.flags();
	originalPrecision_ = os.precision();
	originalWidth_ = os.width();
	loggerFlags_ = originalFlags_;
	loggerPrecision_ = originalPrecision_;
	loggerWidth_ = originalWidth_;
}

/////////

FileLogger::FileLogger(Logging::LogLevel level,
					   const std::string &path, std::ios::openmode mode)
		: Logger(level) {
	file_ = new std::ofstream(path.c_str(), mode);
	updateOS();
}

FileLogger::~FileLogger() {
	delete file_;
}

std::ostream &FileLogger::stream() {
	return *file_;
}

CoutLogger::CoutLogger(Logging::LogLevel level)
		: Logger(level) {
	updateOS();
}

std::ostream &CoutLogger::stream() {
	return std::cout;
}

CerrLogger::CerrLogger(Logging::LogLevel level)
		: Logger(level) {
	updateOS();
}

std::ostream &CerrLogger::stream() {
	return std::cerr;
}
