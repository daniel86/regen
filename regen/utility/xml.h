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
  namespace xml {
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
     * All uppercase attributes are used as shader defines.
     * @param root XML node.
     * @param cfg the shader configuration.
     */
    void loadShaderConfig(rapidxml::xml_node<> *root, StateConfig &cfg);

    /**
     * Load XML node.
     * @param n parent XML node.
     * @param name the node name.
     * @return the XML node
     */
    rapidxml::xml_node<>* loadNode(rapidxml::xml_node<> *n, const string &name);

    /**
     * Load XML attribute.
     * Throw Error if attribute not found.
     * @param n XML node.
     * @param name the attribute name.
     * @return the XML attribute
     */
    rapidxml::xml_attribute<>* loadAttribute(rapidxml::xml_node<> *n, const string &name);

    /**
     * Load XML attribute. Also look at parent nodes for the attribute.
     * With this function attributes can be derived from parent to child.
     * @param n XML node.
     * @param name the attribute name.
     * @return the XML attribute or NULL
     */
    rapidxml::xml_attribute<>* findAttribute(rapidxml::xml_node<> *n, const string &name);

    /**
     * Read attribute value and return in requested type.
     * Throw Error if attribute not found.
     * @param n XML node.
     * @param name attribute name.
     * @return the attribute value.
     */
    template<typename T>
    T readAttribute(rapidxml::xml_node<> *n, const string &name)
    {
      rapidxml::xml_attribute<> *att = findAttribute(n,name);
      if(att==NULL) {
        throw Error( REGEN_STRING("No attribute with name '"<<name<<"' found.") );
      }

      T attValue;
      stringstream ss(att->value());
      ss >> attValue;
      return attValue;
    }

    /**
     * Read attribute value and return in requested type.
     * If attribute value can not be obtained a default value is returned.
     * @param n XML node.
     * @param name attribute name.
     * @return the attribute value.
     */
    template<typename T>
    T readAttribute(rapidxml::xml_node<> *n, const string &name, const T &defaultValue)
    {
      rapidxml::xml_attribute<> *att = findAttribute(n,name);
      if(att==NULL) return defaultValue;

      T attValue;
      stringstream ss(att->value());
      ss >> attValue;
      return attValue;
    }
  }
}

#endif /* __XML_H_ */
