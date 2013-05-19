/*
 * includer.h
 *
 *  Created on: 19.05.2013
 *      Author: daniel
 */

#ifndef INCLUDER_H_
#define INCLUDER_H_

#include <boost/filesystem/path.hpp>

#include <string>
#include <map>
#include <set>
using namespace std;

namespace regen {
  /**
   * \brief Resolves include directives by loading shader code from filesystem.
   */
  class Includer
  {
  public:
    /**
     * @return Singleton Includer instance.
     */
    static Includer& get();

    /**
     * Add a filesystem path to the list of paths that are considered
     * when shader code is included.
     * First added path has precedence over following paths.
     * @param path the filesystem shader lookpup path.
     * @return true on success.
     */
    bool addIncludePath(const string &path);

    /**
     * @param key the include key.
     * @return true if key contains known shader file.
     */
    bool isKeyValid(const string &key);
    /**
     * Include section from shader file.
     * The include key contains sub directory names relative
     * to include path followed by the shader file name
     * and a sequence identifying a section within the file.
     * All items are separated by dots.
     * For example "regen.picking.vs" loads section 'vs' from file
     * 'picking.gls' that is in a child-directory, of an include path,
     * named 'regen'.
     * @param key the include key.
     * @return the shader section or empty string.
     */
    const string& include(const string &key);
    /**
     * @return reason for include failure.
     */
    const string& errorMessage() const;

  protected:
    list<boost::filesystem::path> includePaths_;
    set<string> loadedFiles_;
    map<string,string> sections_;
    string errorMessage_;

    bool parseInput(
        const string &key,
        boost::filesystem::path &filePathRet,
        string &fileKeyRet,
        string &effectKeyRet);
  };
}

#endif /* INCLUDER_H_ */
