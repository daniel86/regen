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
    bool addIncludePath(const std::string &path);

    /**
     * @param key the include key.
     * @return true if key contains known shader file.
     */
    bool isKeyValid(const std::string &key);
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
    const std::string& include(const std::string &key);
    /**
     * @return reason for include failure.
     */
    const std::string& errorMessage() const;

  protected:
    std::list<boost::filesystem::path> includePaths_;
    std::set<std::string> loadedFiles_;
    std::map<std::string,std::string> sections_;
    std::string errorMessage_;

    bool parseInput(
        const std::string &key,
        boost::filesystem::path &filePathRet,
        std::string &fileKeyRet,
        std::string &effectKeyRet);
  };
}

#endif /* INCLUDER_H_ */
