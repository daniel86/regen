#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#ifndef WIN32
#include <pwd.h>
#endif

#include <iostream>

#include "filesystem.h"
#include "regen/config.h"

std::string regen::PathChoice::firstValidPath() {
	for (auto &choice: choices_) {
		if (boost::filesystem::exists(choice)) return choice;
	}
	return *choices_.begin();
}

std::string regen::userDirectory() {
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
	if (home != nullptr) return std::string(home);

	return std::string(getpwuid(getuid())->pw_dir);
#endif
}

std::string regen::filesystemPath(
		const std::string &baseDirectory,
		const std::string &pathString,
		const std::string &separators) {
	boost::filesystem::path p(baseDirectory);

	std::list<std::string> pathNames;
	boost::split(pathNames, pathString, boost::is_any_of(separators));
	for (auto &pathName: pathNames) { p /= pathName; }

	return p.string();
}

std::string regen::resourcePath(const std::string &relPath) {
	PathChoice texPaths;
	texPaths.choices_.push_back(relPath);
	texPaths.choices_.push_back(filesystemPath(
			".", relPath));
	texPaths.choices_.push_back(filesystemPath(
			REGEN_SOURCE_DIR, relPath));
	texPaths.choices_.push_back(filesystemPath(filesystemPath(
			REGEN_SOURCE_DIR, "regen"), relPath));
	texPaths.choices_.push_back(filesystemPath(filesystemPath(
			REGEN_SOURCE_DIR, "applications"), relPath));
	texPaths.choices_.push_back(filesystemPath(filesystemPath(
			REGEN_INSTALL_PREFIX, "share"), relPath));
	return texPaths.firstValidPath();
}
