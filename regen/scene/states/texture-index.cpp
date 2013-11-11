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
#include <regen/scene/states/texture.h>

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

  ref_ptr<Texture> tex = TextureStateProvider::getTexture(parser,input);
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
