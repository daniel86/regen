
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <map>

#include "string-util.h"
using namespace ogle;

bool ogle::hasPrefix(const string &s, const string &prefix)
{
  return boost::starts_with(s, prefix);
}
string ogle::truncPrefix(const string &s, const string &prefix)
{
  if(hasPrefix(s,prefix)) {
    return s.substr(prefix.size());
  } else {
    return s;
  }
}

bool ogle::isInteger(const string & s)
{
  try  {
    boost::lexical_cast<int>(s);
    return true;
  }
  catch(...) {}
  return false;
}
bool ogle::isFloat(const string & s)
{
  try  {
    boost::lexical_cast<double>(s);
    return true;
  }
  catch(...) {}
  return false;
}
bool ogle::isNumber(const string & s)
{
  return isInteger(s) || isFloat(s);
}

