/*
 * texture-index.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "texture-index.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_TEXTURE_INDEX_CATEGORY "texture-index"

TextureIndexProvider::TextureIndexProvider()
: StateProcessor(REGEN_TEXTURE_INDEX_CATEGORY)
{}

void TextureIndexProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  const string texName = input.getValue("name");

  ref_ptr<Texture> tex;
  // Find the texture resource
  if(input.hasAttribute("id")) {
    tex = parser->getResources()->getTexture(parser,input.getValue("id"));
  }
  else if(input.hasAttribute("fbo")) {
    ref_ptr<FBO> fbo = parser->getResources()->getFBO(parser,input.getValue("fbo"));
    if(fbo.get()==NULL) {
      REGEN_WARN("Unable to find FBO '" << input.getValue("fbo") <<
          "' for " << input.getDescription() << ".");
      return;
    }
    const string val = input.getValue<string>("attachment", "0");
    if(val == "depth") {
      tex = fbo->depthTexture();
    }
    else {
      vector< ref_ptr<Texture> > &textures = fbo->colorTextures();

      unsigned int attachment;
      stringstream ss(val);
      ss >> attachment;

      if(attachment < textures.size()) {
        tex = textures[attachment];
      }
      else {
        REGEN_WARN("Invalid attachment '" << val <<
            "' for " << input.getDescription() << ".");
        return;
      }
    }
  }
  if(tex.get()==NULL) {
    REGEN_WARN("Skipping unidentified texture node for " << input.getDescription() << ".");
    return;
  }

  if(input.hasAttribute("value")) {
    GLuint index = input.getValue<GLuint>("index",0u);
    state->joinStates(ref_ptr<TextureSetIndex>::alloc(tex,index));
  }
  else if(input.getValue<bool>("set-next-index",true)) {
    state->joinStates(ref_ptr<TextureNextIndex>::alloc(tex));
  }
  else {
    REGEN_WARN("Skipping " << input.getDescription() << " because no index set.");
  }
}
