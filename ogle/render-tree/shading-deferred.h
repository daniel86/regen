/*
 * deferred-shading.h
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

#ifndef DEFERRED_SHADING_H_
#define DEFERRED_SHADING_H_

#include <list>
using namespace std;

#include <ogle/render-tree/state-node.h>
#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/fbo-state.h>

struct GBufferTarget {
  GBufferTarget(const string &name_, GLenum format_, GLenum internalFormat_)
  : name(name_), internalFormat(internalFormat_), format(format_) {}
  GBufferTarget(const GBufferTarget &other)
  : name(other.name), internalFormat(other.internalFormat), format(other.format) {}
  string name;
  GLenum internalFormat;
  GLenum format;
};

/**
 * Handles deferred shading.
 * First pass renders geometric information
 * to so called GBuffer without handling the lights.
 * The shading can then be calculated using the GBuffer
 * attachments afterwards.
 */
class DeferredShading : public StateNode
{
public:
  DeferredShading(
      GLuint width, GLuint height,
      GLenum depthAttachmentFormat,
      list<GBufferTarget> outputNames);

  /**
   * Number of GBuffer textures.
   */
  GLuint numOutputs() const;
  /**
   * Accumulate lights. Do the actual shading calculations.
   */
  const ref_ptr<StateNode>& accumulationStage() const;
  const ref_ptr<StateNode>& geometryStage() const;

  /**
   * GBuffer's FBO.
   */
  const ref_ptr<FBOState>& framebuffer() const;
  /**
   * Scene depth texture.
   */
  const ref_ptr<Texture>& depthTexture() const;
  /**
   * The GBuffer FBO attachment.
   */
  const ref_ptr<Texture>& colorTexture() const;

  void resize(GLuint w, GLuint h);

protected:
  list<GBufferTarget> outputTargets_;

  ref_ptr<FBOState> framebuffer_;
  ref_ptr<Texture> depthTexture_;
  ref_ptr<Texture> colorTexture_;

  ref_ptr<StateNode> geometryStage_;

  ref_ptr<StateNode> accumulationStage_;
  ref_ptr<ShaderState> accumulationShader_;

  void enable(RenderState *rs);
  void disable(RenderState *rs);
};


#endif /* DEFERRED_SHADING_H_ */
