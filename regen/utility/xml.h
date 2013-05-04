/*
 * xml.h
 *
 *  Created on: 17.03.2013
 *      Author: daniel
 */

#ifndef __XML_H_
#define __XML_H_

#include <stdexcept>
using namespace std;

#include <regen/external/rapidxml/rapidxml.hpp>
#include <regen/external/rapidxml/rapidxml_utils.hpp>
#include <regen/external/rapidxml/rapidxml_print.hpp>
#include <regen/utility/string-util.h>
#include <regen/states/shader-state.h>

namespace regen {
  /**
   * \brief Interface to rapidxml.
   */
  class XMLLoader
  {
  public:
    /**
     * \brief A font related error occurred.
     */
    class Error : public runtime_error {
    public:
      /**
       * @param msg the error message.
       */
      Error(const string& msg) : runtime_error(msg) {}
    };

    /**
     * Loads shader configuration from XML node.
     * All uppercase attributes are used as shader defined.
     * @param root XML node.
     * @param cfg the shader configuration.
     */
    static void loadShaderConfig(rapidxml::xml_node<> *root, StateConfig &cfg)
    {
      // read shader defines (starting upper case)
      for(rapidxml::xml_attribute<>* attr=root->first_attribute(); attr; attr=attr->next_attribute())
      {
        string name = attr->name();
        const char *nameC = name.c_str();
        if(toupper(nameC[0])==nameC[0])
        { cfg.defines_[name] = attr->value(); }
      }
    }

    /**
     * Load XML node.
     * @param root parent XML node.
     * @param name the node name.
     * @return the XML node
     */
    static rapidxml::xml_node<>* loadNode(rapidxml::xml_node<> *root, const string &name)
    {
      rapidxml::xml_node<> *n = root->first_node(name.c_str());
      if(n==NULL) {
        throw Error( REGEN_STRING("No node with name '"<<name<<"' found.") );
      }
      return n;
    }

    /**
     * Load XML attribute.
     * @param root XML node.
     * @param name the attribute name.
     * @return the XML attribute
     */
    static rapidxml::xml_attribute<>* loadAttribute(rapidxml::xml_node<> *root, const string &name)
    {
      rapidxml::xml_attribute<> *a = root->first_attribute(name.c_str());
      if(a==NULL) {
        throw Error( REGEN_STRING("No attribute with name '"<<name<<"' found.") );
      }
      return a;
    }

    /**
     * Read attribute value and return in requested type.
     * @param root XML node.
     * @param name attribute name.
     * @return the attribute value.
     */
    template<typename T>
    static T readAttribute(rapidxml::xml_node<> *root, const string &name)
    {
      T attValue;
      rapidxml::xml_attribute<>* att = XMLLoader::loadAttribute(root,name);
      stringstream ss(att->value());
      ss >> attValue;
      return attValue;
    }
  };
}

#endif /* __XML_H_ */
