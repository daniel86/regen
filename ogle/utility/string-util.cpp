
#include <boost/algorithm/string.hpp>
#include "string-util.h"

bool hasPrefix(const string &s, const string &prefix)
{
  return boost::starts_with(s, prefix);
}
string truncPrefix(const string &s, const string &prefix)
{
  if(hasPrefix(s,prefix)) {
    return s.substr(prefix.size());
  } else {
    return s;
  }
}
