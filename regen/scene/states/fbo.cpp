/*
 * fbo.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "fbo.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_FBO_STATE_CATEGORY "fbo"

static vector<string> getFBOAttachments(ref_ptr<SceneInputNode> n)
{
  vector<string> out;
  string attachments = n->getValue<string>("attachments", "");
  if(attachments.empty()) {
    REGEN_WARN("No attachments specified in " << n->getDescription() << ".");
  } else {
    boost::split(out,attachments,boost::is_any_of(","));
  }
  return out;
}

FBOStateProvider::FBOStateProvider()
: StateProcessor(REGEN_FBO_STATE_CATEGORY)
{}

void FBOStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<FBO> fbo = parser->getResources()->getFBO(parser,input.getName());
  if(fbo.get()==NULL) {
    REGEN_WARN("Unable to find FBO for '" << input.getDescription() << "'.");
    return;
  }
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);

  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;

    if(n->getCategory() == "clear-depth") {
      fboState->setClearDepth();
    }
    else if(n->getCategory() == "clear-buffer") {
      vector<string> idVec = getFBOAttachments(n);
      vector<GLenum> buffers(idVec.size());
      for(GLuint i=0u; i<idVec.size(); ++i) {
        GLint v;
        stringstream ss(idVec[i]);
        ss >> v;
        buffers[i] = GL_COLOR_ATTACHMENT0 + v;
      }

      ClearColorState::Data data;
      data.clearColor = input.getValue<Vec4f>("clear-color", Vec4f(0.0));
      data.colorBuffers = DrawBuffers(buffers);

      fboState->setClearColor(data);
    }
    else if(n->getCategory() == "draw-buffer") {
      vector<string> idVec = getFBOAttachments(n);
      vector<GLenum> buffers(idVec.size());
      for(GLuint i=0u; i<idVec.size(); ++i) {
        GLint v;
        stringstream ss(idVec[i]);
        ss >> v;
        fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0 + v);
      }
    }
    else {
      REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
    }
  }

  state->joinStates(fboState);
}


