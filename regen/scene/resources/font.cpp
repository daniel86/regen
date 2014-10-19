/*
 * font.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "font.h"
using namespace regen::scene;
using namespace regen;
using namespace std;

#define REGEN_FONT_CATEGORY "font"

FontResource::FontResource()
: ResourceProvider(REGEN_FONT_CATEGORY)
{}

ref_ptr<Font> FontResource::createResource(
    SceneParser *parser, SceneInputNode &input)
{
  if(!input.hasAttribute("file")) {
    REGEN_WARN("Ignoring Font '" << input.getDescription() << "' without file.");
    return ref_ptr<Font>();
  }
  return regen::Font::get(
      getResourcePath(input.getValue("file")),
      input.getValue<GLuint>("size", 16u),
      input.getValue<GLuint>("dpi", 96u));
}


