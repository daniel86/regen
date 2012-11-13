/*
 * forward-shading.h
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

#ifndef FORWARD_SHADING_H_
#define FORWARD_SHADING_H_

#include <ogle/render-tree/shading-interface.h>

class ForwardShading : public ShadingInterface
{
public:
  ForwardShading(GLuint width, GLuint height,
      GLenum colorAttachmentFormat,
      GLenum depthAttachmentFormat);

  virtual ref_ptr<StateNode>& geometryStage();
  virtual ref_ptr<FBOState>& framebuffer();
  virtual ref_ptr<Texture>& depthTexture();
  virtual ref_ptr<Texture>& colorTexture();

  // override
  void configureShader(ShaderConfig *cfg);
  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);
protected:
  ref_ptr<FBOState> framebuffer_;
  ref_ptr<Texture> depthTexture_;
  ref_ptr<Texture> colorTexture_;

  ref_ptr<StateNode> geometryStage_;
};

#endif /* FORWARD_SHADING_H_ */
