/*
 * scene-input-xml.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef XML_INPUT_PROVIDER_H_
#define XML_INPUT_PROVIDER_H_

#include <regen/scene/scene-input.h>
#include <regen/utility/xml.h>

namespace regen {
namespace scene {

  class SceneInputNodeXML; // forward declaration

  /**
   * Provides scene input via rapid XML.
   * Note that attributes are inherited from parents
   * to children.
   */
  class SceneInputXML : public SceneInput {
  public:
    /**
     * Default constructor.
     * @param xmlFile The XML input file path.
     */
    SceneInputXML(const string &xmlFile);

    // Override
    ref_ptr<SceneInputNode> getRoot();

  protected:
    ref_ptr<SceneInputNodeXML> rootNode_;
    rapidxml::xml_document<> doc_;
    string inputFile_;
    ifstream xmlInput_;
    vector<char> buffer_;
  };

  /**
   * Provides scene input via rapid XML.
   */
  class SceneInputNodeXML : public SceneInputNode {
  public:
    /**
     * Default constructor.
     * @param parent Parent of the node or null if this is a root node.
     * @param xmlNode A node in the XML tree.
     */
    SceneInputNodeXML(
        SceneInputNodeXML *parent,
        rapidxml::xml_node<> *xmlNode);
    SceneInputNodeXML();

    // Override
    string getCategory();
    string getName();
    const list< ref_ptr<SceneInputNode> >& getChildren();
    const map<string,string>& getAttributes();

  protected:
    rapidxml::xml_node<> *xmlNode_;
    list< ref_ptr<SceneInputNode> > children_;
    map<string,string> attributes_;

    string category_;
    string name_;
  };
}}

#endif /* XML_INPUT_PROVIDER_H_ */
