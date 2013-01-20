/*
 * transparency-state.h
 *
 *  Created on: 03.12.2012
 *      Author: daniel
 */

#ifndef TRANSPARENCY_STATE_H_
#define TRANSPARENCY_STATE_H_

#include <ogle/states/state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>

// TODO: implement other transparency strategies.
//      - order independent
//              * dual depth peeling
//              * stochastic transparency
enum TransparencyMode {
  TRANSPARENCY_MODE_FRONT_TO_BACK,
  TRANSPARENCY_MODE_BACK_TO_FRONT,
  TRANSPARENCY_MODE_SUM,
  TRANSPARENCY_MODE_AVERAGE_SUM,
  TRANSPARENCY_MODE_NONE
};

/**
 * Sets up blending and custom render target for rendering
 * transparent objects.
 */
class TransparencyState : public State
{
public:
  TransparencyState(
      TransparencyMode mode,
      GLuint bufferWidth, GLuint bufferHeight,
      const ref_ptr<Texture> &depthTexture,
      GLboolean useDoublePrecision=GL_FALSE);

  /**
   * Texture with accumulated alpha values.
   */
  const ref_ptr<Texture>& colorTexture() const;
  /**
   * Only used for average sum transparency.
   */
  const ref_ptr<Texture>& counterTexture() const;

  const ref_ptr<FBOState>& fboState() const;

  void resize(GLuint bufferWidth, GLuint bufferHeight);

  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);

protected:
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<FBOState> fboState_;
  ref_ptr<Texture> colorTexture_;
  ref_ptr<Texture> counterTexture_;
};

///////////
//////////

#include <ogle/render-tree/state-node.h>
#include <ogle/states/shader-state.h>
#include <ogle/meshes/mesh-state.h>

class AccumulateTransparency : public StateNode
{
public:
  AccumulateTransparency(
      TransparencyMode transparencyMode,
      const ref_ptr<FrameBufferObject> &fbo,
      const ref_ptr<Texture> &colorTexture);
  ~AccumulateTransparency();

  void setTransparencyTextures(const ref_ptr<Texture> &color, const ref_ptr<Texture> &counter);
  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);

protected:
  ref_ptr<FBOState> fbo_;
  ref_ptr<Texture> colorTexture_;
  ref_ptr<ShaderState> accumulationShader_;
  ref_ptr<Texture> alphaColorTexture_;
  ref_ptr<Texture> alphaCounterTexture_;
  GLint *outputChannels_;
};

#endif /* TRANSPARENCY_STATE_H_ */
