/*
 * xml.h
 *
 *  Created on: 17.03.2013
 *      Author: daniel
 */

#ifndef XML_H_
#define XML_H_

#include <stdexcept>
using namespace std;

#include <ogle/external/rapidxml/rapidxml.hpp>
#include <ogle/external/rapidxml/rapidxml_utils.hpp>
#include <ogle/external/rapidxml/rapidxml_print.hpp>
#include <ogle/utility/string-util.h>

namespace ogle {
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

  static void loadShaderConfig(rapidxml::xml_node<> *root, ShaderState::Config &cfg)
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

  static rapidxml::xml_node<>* loadNode(rapidxml::xml_node<> *root, const string &name)
  {
    rapidxml::xml_node<> *n = root->first_node(name.c_str());
    if(n==NULL) {
      throw Error( FORMAT_STRING("No node with name '"<<name<<"' found.") );
    }
    return n;
  }

  static rapidxml::xml_attribute<>* loadAttribute(rapidxml::xml_node<> *root, const string &name)
  {
    rapidxml::xml_attribute<> *a = root->first_attribute(name.c_str());
    if(a==NULL) {
      throw Error( FORMAT_STRING("No attribute with name '"<<name<<"' found.") );
    }
    return a;
  }

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

#endif /* XML_H_ */
