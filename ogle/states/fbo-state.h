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
#include <ogle/shader/shader-fragment-output.h>

struct ClearColorData {
  Vec4f clearColor;
  GLenum colorAttachment;
};

class ClearDepthState : public Callable
{
public:
  ClearDepthState();
  virtual void call();
};
class ClearColorState : public Callable
{
public:
  ClearColorState();
  virtual void call();
  list<ClearColorData> data;
};
class DrawBufferState : public Callable
{
public:
  DrawBufferState();
  virtual void call();
  vector<GLuint> colorBuffers;
};

/**
 * Provides Framebuffer Object render target
 * and related states.
 */
class FBOState : public State
{
public:
  FBOState(ref_ptr<FrameBufferObject> &fbo);

  void resize(GLuint width, GLuint height);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
  virtual void configureShader(ShaderConfiguration*);

  void setClearDepth();

  void setClearColor(const ClearColorData &data);
  void setClearColor(const list<ClearColorData> &data);

  ref_ptr<Texture> addDrawBuffer(
      ref_ptr<ShaderFragmentOutput> fragmentShaderOutput);

  ref_ptr<Texture> addDefaultDrawBuffer(
      bool pingPongBuffer, GLenum colorAttachment);

  ref_ptr<FrameBufferObject>& fbo();

  virtual string name();
protected:
  ref_ptr<FrameBufferObject> fbo_;

  list< ref_ptr<ShaderFragmentOutput> > fragmentOutputs_;

  ref_ptr<ClearDepthState> clearDepthCallable_;
  ref_ptr<ClearColorState> clearColorCallable_;
  ref_ptr<DrawBufferState> drawBufferCallable_;

  ref_ptr<ShaderInput2f> viewportUniform_;
};

#endif /* FBO_NODE_H_ */
