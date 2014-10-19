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

std::string regen::PathChoice::firstValidPath()
{
  for(std::list<std::string>::iterator it=choices_.begin(); it!=choices_.end(); ++it)
  {
    if(boost::filesystem::exists(*it)) return *it;
  }
  return *choices_.begin();
}

std::string regen::userDirectory()
{
#ifdef WIN32
  char *userProfile = getenv("USERPROFILE");
  if(userProfile!=NULL) return string(userProfile);

  char *homeDrive = getenv("HOMEDRIVE");
  char *homePath = getenv("HOMEPATH");
  if(homeDrive!=NULL) {
    if(homePath!=NULL) {
      return std::string(homeDrive) + std::string(homePath);
    } else {
      return std::string(homeDrive);
    }
  } else {
    return "C:";
  }
#else
  char *home = getenv("HOME");
  if(home!=NULL) return std::string(home);

  return std::string( getpwuid(getuid())->pw_dir );
#endif
}

std::string regen::filesystemPath(
    const std::string &baseDirectory,
    const std::string &pathString,
    const std::string &separators)
{
  boost::filesystem::path p(baseDirectory);

  std::list<std::string> pathNames;
  boost::split(pathNames, pathString, boost::is_any_of(separators));
  for(std::list<std::string>::iterator it=pathNames.begin(); it!=pathNames.end(); ++it)
  { p /= (*it); }

  return p.string();
}
