/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

#include <ogle/states/polygon-offset-state.h>
#include <ogle/states/cull-state.h>

static void traverseTree(RenderState *rs, StateNode *node)
{
  if(rs->isNodeHidden(node)) { return; }

  node->enable(rs);
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    traverseTree(rs, it->get());
  }
  node->disable(rs);
}

class ShadowRenderState : public RenderState
{
public:
  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}
};

ShadowMap::ShadowMap()
: Animation(), State()
{
  //setCullFrontFaces(GL_TRUE);
  setPolygonOffset();
}

void ShadowMap::setPolygonOffset(GLfloat factor, GLfloat units)
{
  if(polygonOffsetState_.get()) {
    disjoinStates(polygonOffsetState_);
  }
  polygonOffsetState_ = ref_ptr<State>::manage(new PolygonOffsetState(factor,units));
  joinStates(polygonOffsetState_);
}

void ShadowMap::setCullFrontFaces(GLboolean v)
{
  if(cullState_.get()) {
    disjoinStates(cullState_);
  }
  if(v) {
    cullState_ = ref_ptr<State>::manage(new CullFrontFaceState);
    joinStates(cullState_);
  } else {
    cullState_ = ref_ptr<State>();
  }
}

void ShadowMap::addCaster(ref_ptr<StateNode> &caster)
{
  caster_.push_back(caster);
}
void ShadowMap::removeCaster(StateNode *caster)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ref_ptr<StateNode> &n = *it;
    if(n.get()==caster) {
      caster_.erase(it);
      break;
    }
  }
}

void ShadowMap::traverse()
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ShadowRenderState rs;
    traverseTree(&rs, it->get());
  }
}

void ShadowMap::animate(GLdouble dt)
{
}

void ShadowMap::updateGraphics(GLdouble dt)
{
  // TODO: glColorMask
  ShadowRenderState rs;
  enable(&rs);
  updateShadow();
  disable(&rs);
}

void ShadowMap::drawDebugHUD(
    GLenum textureTarget,
    GLenum textureCompareMode,
    GLuint numTextures,
    GLuint textureID,
    const string &fragmentShader)
{
  if(debugShader_.get() == NULL) {
    debugShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    map<string, string> shaderConfig;
    map<GLenum, string> shaderNames;
    shaderNames[GL_FRAGMENT_SHADER] = fragmentShader;
    shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.debug.vs";
    debugShader_->createSimple(shaderConfig,shaderNames);
    debugShader_->shader()->compile();
    debugShader_->shader()->link();

    debugLayerLoc_ = glGetUniformLocation(debugShader_->shader()->id(), "in_shadowLayer");
    debugTextureLoc_ = glGetUniformLocation(debugShader_->shader()->id(), "in_shadowMap");
  }

  glDisable(GL_DEPTH_TEST);
  glUseProgram(debugShader_->shader()->id());
  glUniform1i(debugTextureLoc_, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(textureTarget, textureID);
  glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);

  for(GLuint i=0; i<numTextures; ++i) {
    glViewport(130*i, 0, 128, 128);
    glUniform1f(debugLayerLoc_, float(i));

    glBegin(GL_QUADS);
    glVertex3f(-1.0, -1.0, 0.0);
    glVertex3f( 1.0, -1.0, 0.0);
    glVertex3f( 1.0,  1.0, 0.0);
    glVertex3f(-1.0,  1.0, 0.0);
    glEnd();
  }

  // reset states
  glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, textureCompareMode);
  glEnable(GL_DEPTH_TEST);
}
