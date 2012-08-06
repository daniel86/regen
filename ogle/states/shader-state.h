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

class ShaderState : public State
{
public:
  ShaderState(ref_ptr<Shader> &shader);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

  ref_ptr<Shader> shader();
protected:
  ref_ptr<Shader> shader_;
};

#endif /* SHADER_NODE_H_ */
