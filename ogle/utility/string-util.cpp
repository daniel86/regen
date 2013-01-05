
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <map>

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

bool isInteger(const string & s)
{
  try  {
    boost::lexical_cast<int>(s);
    return true;
  }
  catch(...) { return false; }
}
bool isFloat(const string & s)
{
  try  {
    boost::lexical_cast<double>(s);
    return true;
  }
  catch(...) { return false; }
}
bool isNumber(const string & s)
{
  return isInteger(s) || isFloat(s);
}

