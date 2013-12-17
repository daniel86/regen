/*
 * fbo.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_FBO_H_
#define REGEN_SCENE_FBO_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>

#define REGEN_FBO_STATE_CATEGORY "fbo"

static vector<string> getFBOAttachments(SceneInputNode &input, const string &key)
{
  vector<string> out;
  string attachments = input.getValue<string>(key, "");
  if(attachments.empty()) {
    REGEN_WARN("No attachments specified in " << input.getDescription() << ".");
  } else {
    boost::split(out,attachments,boost::is_any_of(","));
  }
  return out;
}

#include <regen/states/fbo-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates FBOState's.
   */
  class FBOStateProvider : public StateProcessor {
  public:
    FBOStateProvider()
    : StateProcessor(REGEN_FBO_STATE_CATEGORY)
    {}

    // Override
    void processInput(
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

      if(input.hasAttribute("clear-depth") &&
         input.getValue<bool>("clear-depth", true))
      {
        fboState->setClearDepth();
      }

      if(input.hasAttribute("clear-buffers")) {
        vector<string> idVec = getFBOAttachments(input,"clear-buffers");
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

      if(input.hasAttribute("draw-buffers")) {
        vector<string> idVec = getFBOAttachments(input,"draw-buffers");
        vector<GLenum> buffers(idVec.size());
        for(GLuint i=0u; i<idVec.size(); ++i) {
          GLint v;
          stringstream ss(idVec[i]);
          ss >> v;
          buffers[i] = GL_COLOR_ATTACHMENT0 + v;
        }
        fboState->setDrawBuffers(buffers);
      }
      else if(input.hasAttribute("ping-pong-buffers")) {
        vector<string> idVec = getFBOAttachments(input,"ping-pong-buffers");
        vector<GLenum> buffers(idVec.size());
        for(GLuint i=0u; i<idVec.size(); ++i) {
          GLint v;
          stringstream ss(idVec[i]);
          ss >> v;
          buffers[i] = GL_COLOR_ATTACHMENT0 + v;
        }
        fboState->setPingPongBuffers(buffers);
      }

      state->joinStates(fboState);
    }
  };
}}

#endif /* REGEN_SCENE_FBO_H_ */
