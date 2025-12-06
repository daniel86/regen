#include <regen/config.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "strings.h"

using namespace regen;
using namespace std;

bool regen::hasPrefix(const string &s, const string &prefix) {
	return boost::starts_with(s, prefix);
}

string regen::truncPrefix(const string &s, const string &prefix) {
	if (hasPrefix(s, prefix)) {
		return s.substr(prefix.size());
	} else {
		return s;
	}
}

bool regen::isInteger(const string &s) {
	try {
		boost::lexical_cast<int>(s);
		return true;
	}
	catch (...) {}
	return false;
}

bool regen::isFloat(const string &s) {
	try {
		boost::lexical_cast<double>(s);
		return true;
	}
	catch (...) {}
	return false;
}

bool regen::isNumber(const string &s) {
	return isInteger(s) || isFloat(s);
}
