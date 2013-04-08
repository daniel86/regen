/*
 * filesystem.h
 *
 *  Created on: 08.04.2013
 *      Author: daniel
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

using namespace std;
#include <string>
#include <list>

namespace regen {
/**
 * @return the user base directory.
 */
string userDirectory();

/**
 * \brief A choice of multiple paths.
 */
struct PathChoice {
  /**
   * The list of paths.
   */
  list<string> choices_;
  /**
   * @return first path in the choices list that exists or an empty string if none exists.
   */
  string firstValidPath();
};

/**
 * Build a filesystem path.
 * @param baseDirectory the base directory. It's prepended as is.
 * @param pathString the path string. Directory names are separated by one of the specified separators.
 * @param separators string that contains separator characters.
 * @return the filesystem path.
 */
string filesystemPath(
    const string &baseDirectory,
    const string &pathString,
    const string &separators="/");
};

#endif /* FILESYSTEM_H_ */
