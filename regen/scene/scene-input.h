/*
 * scene-input.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef SCENE_INPUT_H_
#define SCENE_INPUT_H_

#include <regen/utility/ref-ptr.h>
#include <regen/utility/logging.h>
#include <regen/gl-types/gl-enum.h>
#include <string>
#include <list>
#include <vector>
#include <map>
using namespace std;

namespace regen {
namespace scene {
  class SceneInputNode; // forward declaration

  /**
   * Provides input to SceneParser.
   */
  class SceneInput {
  public:
    virtual ~SceneInput() {};
    /**
     * @return Root of scene graph input.
     */
    virtual ref_ptr<SceneInputNode> getRoot() = 0;
    /**
     * @param nodeCategory Category of returned node.
     * @param nodeName Name of returned node.
     * @return The node or null.
     */
    ref_ptr<SceneInputNode> getNode(
        const string &nodeCategory,
        const string &nodeName);
  };

  /**
   * Provides input to SceneParser.
   */
  class SceneInputNode {
  public:
    SceneInputNode(SceneInputNode *parent);
    virtual ~SceneInputNode() {};

    /**
     * @return Input category identifier.
     */
    virtual string getCategory() = 0;
    /**
     * @return Input identifier (should be unique for category).
     */
    virtual string getName() = 0;

    /**
     * @return List of node children.
     */
    virtual const list< ref_ptr<SceneInputNode> >& getChildren() = 0;

    /**
     * @return Map of InputNode attributes.
     */
    virtual const map<string,string>& getAttributes() = 0;

    /**
     * @return Description intended for debugging.
     */
    string getDescription();

    ///////////////
    ///////////////

    /**
     * @param category Category of returned nodes.
     * @return list of node children.
     */
    list< ref_ptr<SceneInputNode> > getChildren(const std::string &category);
    /**
     * @param category Category of returned nodes.
     * @param name Name of returned nodes.
     * @return node child or null.
     */
    list< ref_ptr<SceneInputNode> > getChildren(const string &category, const string &name);
    /**
     * @param category Category of returned nodes.
     * @param name Name of returned nodes.
     * @return list of node children.
     */
    ref_ptr<SceneInputNode> getFirstChild(const string &category, const string &name);
    /**
     * @param category Category of returned nodes.
     * @return list of node children.
     */
    ref_ptr<SceneInputNode> getFirstChild(const string &category);

    /**
     * @param key the attribute key.
     * @return true if attibute found.
     */
    bool hasAttribute(const string &key);

    /**
     * @param key the attribute key.
     * @return Get the attribute value or empty string if attribute
     *          not found.
     */
    const string& getValue(const string &key);

    /**
     * @param key the attribute key.
     * @param defaultValue A default value.
     * @return Get the typed attribute value or the default value if attribute
     *          not found.
     */
    template<class T> T getValue(const string &key, const T &defaultValue) {
      if(hasAttribute(key)) {
        T attValue;
        stringstream ss(getValue(key));
        if((ss >> attValue).fail()) {
          REGEN_WARN("Failed to parse node value '" << getValue(key) <<
              "' for node " << getDescription() << ".");
          return defaultValue;
        }
        else {
          return attValue;
        }
      }
      else {
        if(parent_!=NULL) {
          return parent_->getValue<T>(key,defaultValue);
        }
        else {
          return defaultValue;
        }
      }
    }

    /**
     * Parse attributes describing a set of indices.
     * Following keys are handled:
     *          'index': A single index
     *          'indices': List of comma separated indices
     *          'from-index': Start of index range
     *          'to-index': Stop of index range
     *          'index-step': Step size for processing index range
     *          'random-indices': Number of random indices to choose
     * If no known key is available, all indices are choosen.
     * @param maxIndex Maximal index in output list.
     * @return The list of indices.
     */
    list<GLuint> getIndexSequence(GLuint maxIndex);

  protected:
    SceneInputNode *parent_;
  };
}}


#endif /* SCENE_INPUT_H_ */
