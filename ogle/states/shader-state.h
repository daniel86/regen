/*
 * shader-node.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SHADER_NODE_H_
#define SHADER_NODE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/shader.h>

/**
 * Provides shader program for child states.
 */
class ShaderState : public State
{
public:
  ShaderState(ref_ptr<Shader> shader);
  ShaderState();

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

  ref_ptr<Shader> shader();
  void set_shader(ref_ptr<Shader> shader);

  virtual string name();
protected:
  ref_ptr<Shader> shader_;
};

#endif /* SHADER_NODE_H_ */
