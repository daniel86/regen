/*
 * xml.cpp
 *
 *  Created on: 05.05.2013
 *      Author: daniel
 */

#include "xml.h"

using namespace regen;
using namespace std;

void xml::loadShaderConfig(rapidxml::xml_node<> *root, StateConfig &cfg) {
	// read shader defines (starting upper case)
	for (rapidxml::xml_attribute<> *attr = root->first_attribute(); attr; attr = attr->next_attribute()) {
		std::string name = attr->name();
		const char *nameC = name.c_str();
		if (toupper(nameC[0]) == nameC[0]) { cfg.defines_[name] = attr->value(); }
	}
}

rapidxml::xml_attribute<> *xml::findAttribute(rapidxml::xml_node<> *root, const string &name) {
	rapidxml::xml_attribute<> *att = nullptr;
	for (rapidxml::xml_node<> *n = root; n != nullptr; n = n->parent()) {
		att = n->first_attribute(name.c_str());
		if (att != nullptr) break;
	}
	return att;
}

rapidxml::xml_node<> *xml::loadNode(rapidxml::xml_node<> *root, const string &name) {
	rapidxml::xml_node<> *n = root->first_node(name.c_str());
	if (n == nullptr) {
		throw Error(REGEN_STRING("No node with name '" << name << "' found."));
	}
	return n;
}

rapidxml::xml_attribute<> *xml::loadAttribute(rapidxml::xml_node<> *root, const string &name) {
	rapidxml::xml_attribute<> *a = root->first_attribute(name.c_str());
	if (a == nullptr) {
		throw Error(REGEN_STRING("No attribute with name '" << name << "' found."));
	}
	return a;
}
