/*
 * forward-shading.cpp
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

/*
 * deferred-shading.cpp
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

#include "shading-forward.h"

ForwardShading::ForwardShading(
    GLuint width, GLuint height,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat)
: ShadingInterface()
{
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(width,height,depthAttachmentFormat));
  framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
  ClearColorData colorData;
  colorData.clearColor = Vec4f(0.0f);
  colorData.colorBuffers.push_back( GL_COLOR_ATTACHMENT0 );
  framebuffer_->addDrawBuffer(GL_COLOR_ATTACHMENT0);
  framebuffer_->setClearColor(colorData);
  depthTexture_ = ref_ptr<Texture>::cast(framebuffer_->fbo()->depthTexture());
  colorTexture_ = framebuffer_->fbo()->addTexture(2,GL_RGBA,colorAttachmentFormat);

  // first render geometry and material info to GBuffer
  geometryStage_ = ref_ptr<StateNode>::manage(new StateNode);
  addChild(geometryStage_);

  state_->shaderDefine("USE_DEFERRED_SHADING", "FALSE");
  state_->shaderDefine("USE_FORWARD_SHADING", "TRUE");
}

void ForwardShading::enable(RenderState *rs) {
  framebuffer()->enable(rs);
  StateNode::enable(rs);
}
void ForwardShading::disable(RenderState *rs) {
  StateNode::disable(rs);
  framebuffer()->disable(rs);
}

ref_ptr<FBOState>& ForwardShading::framebuffer()
{
  return framebuffer_;
}
ref_ptr<Texture>& ForwardShading::depthTexture()
{
  return depthTexture_;
}
ref_ptr<Texture>& ForwardShading::colorTexture()
{
  return colorTexture_;
}
ref_ptr<StateNode>& ForwardShading::geometryStage()
{
  return geometryStage_;
}
