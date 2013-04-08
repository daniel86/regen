/*
 * filesystem.cpp
 *
 *  Created on: 08.04.2013
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#ifndef WIN32
#include <pwd.h>
#endif

#include <iostream>

#include "filesystem.h"

string regen::PathChoice::firstValidPath()
{
  for(list<string>::iterator it=choices_.begin(); it!=choices_.end(); ++it)
  {
    if(boost::filesystem::exists(*it)) return *it;
  }
  return "";
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

string regen::filesystemPath(
    const string &baseDirectory,
    const string &pathString,
    const string &separators)
{
  boost::filesystem::path p(baseDirectory);

  list<string> pathNames;
  boost::split(pathNames, pathString, boost::is_any_of(separators));
  for(list<string>::iterator it=pathNames.begin(); it!=pathNames.end(); ++it)
  { p /= (*it); }

  return p.string();
}
