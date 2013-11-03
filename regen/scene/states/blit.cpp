/*
 * blit.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "blit.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_BLIT_STATE_CATEGORY "blit"

BlitStateProvider::BlitStateProvider()
: StateProcessor(REGEN_BLIT_STATE_CATEGORY)
{}

void BlitStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  if(!input.hasAttribute("src-fbo")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without src-fbo attribute.");
    return;
  }
  if(!input.hasAttribute("dst-fbo")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without dst-fbo attribute.");
    return;
  }
  const string srcID = input.getValue("src-fbo");
  const string dstID = input.getValue("dst-fbo");
  ref_ptr<FBO> src, dst;
  if(srcID != "SCREEN") {
    src = parser->getResources()->getFBO(parser,srcID);
    if(src.get()==NULL) {
      REGEN_WARN("Unable to find FBO with name '" << srcID << "'.");
      return;
    }
  }
  if(dstID != "SCREEN") {
    dst = parser->getResources()->getFBO(parser,dstID);
    if(dst.get()==NULL) {
      REGEN_WARN("Unable to find FBO with name '" << dstID << "'.");
      return;
    }
  }
  if(src.get()!=NULL && dst.get()!=NULL) {
    GLuint srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
    GLuint dstAttachment = input.getValue<GLuint>("dst-attachment", 0u);
    state->joinStates(ref_ptr<BlitToFBO>::alloc(
        src, dst,
        GL_COLOR_ATTACHMENT0+srcAttachment,
        GL_COLOR_ATTACHMENT0+dstAttachment));
  }
  else if(src.get()!=NULL) {
    // Blit Texture to Screen
    GLuint srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
    state->joinStates(ref_ptr<BlitToScreen>::alloc(src,
        parser->getViewport(), GL_COLOR_ATTACHMENT0+srcAttachment));
  }
  else if(dst.get()!=NULL) {
    REGEN_WARN(input.getDescription() << ", blitting Screen to FBO not supported.");
  }
  else {
    REGEN_WARN("No src or dst FBO specified for " << input.getDescription() << ".");
  }
}
