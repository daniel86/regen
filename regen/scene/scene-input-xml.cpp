/*
 * scene-input-xml.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "scene-input-xml.h"

using namespace regen::scene;
using namespace regen;

SceneInputXML::SceneInputXML(const string &xmlFile)
{
  inputFile_ = xmlFile;
  xmlInput_.open(inputFile_.c_str(), ios::in);
  buffer_ = vector<char>((
          istreambuf_iterator<char>(xmlInput_)),
          istreambuf_iterator<char>());
  buffer_.push_back('\0');
  doc_.parse<0>( &buffer_[0] );

  SceneInputNodeXML *nullParent = NULL;
  rootNode_ = ref_ptr<SceneInputNodeXML>::alloc(nullParent,&doc_);
}

ref_ptr<SceneInputNode> SceneInputXML::getRoot()
{ return rootNode_; }

////////////////////
////////////////////

SceneInputNodeXML::SceneInputNodeXML(
    SceneInputNodeXML *parent,
    rapidxml::xml_node<> *xmlNode)
: SceneInputNode(parent),
  xmlNode_(xmlNode)
{
  category_ = xmlNode_->name();

  rapidxml::xml_attribute<> *a = xmlNode_->first_attribute("id");
  if(a!=NULL) {
    name_ = a->value();
  } else {
    name_ = REGEN_STRING(rand());
  }
}

string SceneInputNodeXML::getCategory()
{ return category_; }
string SceneInputNodeXML::getName()
{ return name_; }

const list< ref_ptr<SceneInputNode> >& SceneInputNodeXML::getChildren()
{
  if(children_.empty()) {
    for(rapidxml::xml_node<> *n=xmlNode_->first_node(); n!=NULL; n= n->next_sibling())
    {
      children_.push_back(ref_ptr<SceneInputNodeXML>::alloc(this,n));
    }
  }
  return children_;
}

const map<string,string>& SceneInputNodeXML::getAttributes()
{
  if(attributes_.empty()) {
    for(rapidxml::xml_attribute<> *a=xmlNode_->first_attribute(); a!=NULL; a= a->next_attribute())
    {
      attributes_[a->name()] = a->value();
    }
  }
  return attributes_;
}
