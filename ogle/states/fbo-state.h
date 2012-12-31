/*
 * fbo-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef FBO_NODE_H_
#define FBO_NODE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/fbo.h>

struct ClearColorData {
  Vec4f clearColor;
  vector<GLenum> colorBuffers;
};

/**
 * Clears the depth buffer.
 */
class ClearDepthState : public State
{
public:
  ClearDepthState();
  virtual void enable(RenderState *state);
};
/**
 * Clears color attachments.
 */
class ClearColorState : public State
{
public:
  ClearColorState();
  virtual void enable(RenderState *state);
  list<ClearColorData> data;
};
/**
 * Sets up draw buffers.
 */
class DrawBufferState : public State
{
public:
  DrawBufferState();
  virtual void enable(RenderState *state);
  vector<GLenum> colorBuffers;
};

/**
 * Framebuffer Objects are a mechanism for rendering to images
 * other than the default OpenGL Default Framebuffer.
 * They are OpenGL Objects that allow you to render directly
 * to textures, as well as blitting from one framebuffer to another.
 */
class FBOState : public State
{
public:
  FBOState(const ref_ptr<FrameBufferObject> &fbo);
  virtual ~FBOState() {}

  /**
   * Resize attached textures.
   */
  void resize(GLuint width, GLuint height);

  /**
   * Call if enabling this FBO should clear the attached depth buffer.
   */
  void setClearDepth();

  /**
   * Sets buffers to clear when FBO is enabled.
   */
  void setClearColor(const ClearColorData &data);
  /**
   * Sets buffers to clear when FBO is enabled.
   */
  void setClearColor(const list<ClearColorData> &data);

  /**
   * Add a draw buffer to the enabled draw buffers
   * when the FBO is enabled.
   */
  void addDrawBuffer(GLenum colorAttachment);
  /**
   */
  ref_ptr<Texture> addDefaultDrawBuffer(GLboolean pingPongBuffer, GLenum colorAttachment);

  vector<GLenum> drawBuffers();

  const ref_ptr<FrameBufferObject>& fbo();

  // override
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
protected:
  ref_ptr<FrameBufferObject> fbo_;

  ref_ptr<ClearDepthState> clearDepthCallable_;
  ref_ptr<ClearColorState> clearColorCallable_;
  ref_ptr<DrawBufferState> drawBufferCallable_;

  ref_ptr<ShaderInput2f> viewportUniform_;
};

#endif /* FBO_NODE_H_ */
