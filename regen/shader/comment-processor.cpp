#include "regen/utility/logging.h"
#include <boost/regex.hpp>
#include "comment-processor.h"
using namespace regen;

#define NO_REGEX_MATCH boost::sregex_iterator()

CommentProcessor::CommentProcessor()
	: GLSLProcessor("Comment"),
	  commentActive_(false) {
}

void CommentProcessor::clear() {
	commentActive_ = false;
}

bool CommentProcessor::process(PreProcessorState &state, std::string &line) {
	if (!getlineParent(state, line)) return false;

	static const char *pattern = "(\\/\\/|\\/\\*|\\*\\/|\\-\\-)";
	static boost::regex regex(pattern);

	boost::sregex_iterator it(line.begin(), line.end(), regex), rend, last;
	if (it == rend) {
		if (commentActive_) {
			// skip comment lines
			return getline(state, line);
		} else {
			// no comment active
			return true;
		}
	}
	std::stringstream lineStream;

	for (; it != rend; ++it) {
		const std::string &comment = (*it)[1];

		if (commentActive_) {
			if (comment == "*/") {
				// multi line comment end
				commentActive_ = false;
			}
		} else {
			lineStream << it->prefix();
			if (comment == "/*") {
				// multi line comment start
				commentActive_ = true;
			} else if (comment == "//" || comment == "--") {
				// single line comment start
				line = lineStream.str();
				if (line.empty()) {
					return getline(state, line);
				} else {
					return true;
				}
			}
		}
		last = it;
	}
	if (!commentActive_) {
		lineStream << last->suffix();
	}
	line = lineStream.str();
	if (line.empty()) {
		return getline(state, line);
	} else {
		return true;
	}
}
