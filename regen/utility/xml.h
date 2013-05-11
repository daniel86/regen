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
     * Loads configuration from XML node.
     * All uppercase attributes are used as defines.
     * @param root XML node.
     * @param cfg the state configuration.
     */
    void loadStateConfig(rapidxml::xml_node<> *root, StateConfig &cfg);

    /**
     * Load XML node.
     * @param root parent XML node.
     * @param name the node name.
     * @return the XML node
     */
    rapidxml::xml_node<>* loadNode(rapidxml::xml_node<> *root, const string &name);

    /**
     * Load XML attribute.
     * @param root XML node.
     * @param name the attribute name.
     * @return the XML attribute
     */
    rapidxml::xml_attribute<>* loadAttribute(rapidxml::xml_node<> *root, const string &name);

    /**
     * Read attribute value and return in requested type using >> operator.
     * @param root XML node.
     * @param name attribute name.
     * @return the attribute value.
     */
    template<typename T>
    T readAttribute(rapidxml::xml_node<> *root, const string &name)
    {
      T attValue;
      rapidxml::xml_attribute<>* att = loadAttribute(root,name);
      stringstream ss(att->value());
      ss >> attValue;
      return attValue;
    }
  }
}

#endif /* __XML_H_ */
