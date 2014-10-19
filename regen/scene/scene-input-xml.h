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
    SceneInputXML(const std::string &xmlFile);

    // Override
    ref_ptr<SceneInputNode> getRoot();

  protected:
    ref_ptr<SceneInputNodeXML> rootNode_;
    rapidxml::xml_document<> doc_;
    std::string inputFile_;
    std::ifstream xmlInput_;
    std::vector<char> buffer_;
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
    std::string getCategory();
    std::string getName();
    const std::list< ref_ptr<SceneInputNode> >& getChildren();
    const std::map<std::string,std::string>& getAttributes();

  protected:
    rapidxml::xml_node<> *xmlNode_;
    std::list< ref_ptr<SceneInputNode> > children_;
    std::map<std::string,std::string> attributes_;
    std::list< ref_ptr<SceneInputXML> > inclusions_;

    std::string category_;
    std::string name_;
  };
}}

#endif /* XML_INPUT_PROVIDER_H_ */
