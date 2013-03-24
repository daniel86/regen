/*
 * fbo-state.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef __FBO_STATE_H_
#define __FBO_STATE_H_

#include <regen/states/state.h>
#include <regen/states/atomic-states.h>
#include <regen/gl-types/fbo.h>

namespace regen {
/**
 * \brief Framebuffer Objects are a mechanism for rendering to images
 * other than the default OpenGL Default Framebuffer.
 *
 * They are OpenGL Objects that allow you to render directly
 * to textures, as well as blitting from one framebuffer to another.
 */
class FBOState : public State
{
public:
  /**
   * @param fbo FBO instance.
   */
  FBOState(const ref_ptr<FrameBufferObject> &fbo);

  /**
   * @return the FrameBufferObject instance.
   */
  const ref_ptr<FrameBufferObject>& fbo();

  /**
   * Resize attached textures.
   */
  void resize(GLuint width, GLuint height);

  /**
   * Clear depth buffer to preset values.
   */
  void setClearDepth();
  /**
   * Clear color buffer to preset values.
   */
  void setClearColor(const ClearColorState::Data &data);
  /**
   * Clear color buffers to preset values.
   */
  void setClearColor(const list<ClearColorState::Data> &data);

  /**
   * Add a draw buffer to the list of color buffers to be drawn into.
   */
  void addDrawBuffer(GLenum colorAttachment);
  /**
   * Draw on-top of a single attachment.
   */
  void setDrawBufferOntop(const ref_ptr<Texture> &tex, GLenum baseAttachment);
  /**
   * Ping-pong rendering with two color attachments.
   */
  void setDrawBufferUpdate(const ref_ptr<Texture> &tex, GLenum baseAttachment);

  // override
  void enable(RenderState*);
  void disable(RenderState*);

protected:
  ref_ptr<FrameBufferObject> fbo_;

  ref_ptr<ClearDepthState> clearDepthCallable_;
  ref_ptr<ClearColorState> clearColorCallable_;
  ref_ptr<State> drawBufferCallable_;

  GLboolean useMRT_;
};
} // namespace

#endif /* __FBO_STATE_H_ */
