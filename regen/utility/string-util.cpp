
#include <regen/config.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#ifndef WIN32
#include <pwd.h>
#endif

#include <map>

#include "string-util.h"
using namespace regen;

bool regen::hasPrefix(const string &s, const string &prefix)
{
  return boost::starts_with(s, prefix);
}
string regen::truncPrefix(const string &s, const string &prefix)
{
  if(hasPrefix(s,prefix)) {
    return s.substr(prefix.size());
  } else {
    return s;
  }
}

bool regen::isInteger(const string & s)
{
  try  {
    boost::lexical_cast<int>(s);
    return true;
  }
  catch(...) {}
  return false;
}
bool regen::isFloat(const string & s)
{
  try  {
    boost::lexical_cast<double>(s);
    return true;
  }
  catch(...) {}
  return false;
}
bool regen::isNumber(const string & s)
{
  return isInteger(s) || isFloat(s);
}
string regen::userDirectory()
{
#ifdef WIN32
  char *userProfile = getenv("USERPROFILE");
  if(userProfile!=NULL) return string(userProfile);

  char *homeDrive = getenv("HOMEDRIVE");
  char *homePath = getenv("HOMEPATH");
  if(homeDrive!=NULL) {
    if(homePath!=NULL) {
      return string(homeDrive) + string(homePath);
    } else {
      return string(homeDrive);
    }
  } else {
    return "C:";
  }
#else
  char *home = getenv("HOME");
  if(home!=NULL) return string(home);

  return string( getpwuid(getuid())->pw_dir );
#endif
}

