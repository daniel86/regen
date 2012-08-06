/*
 * uniform-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef UNIFORM_NODE_H_
#define UNIFORM_NODE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/uniform.h>

class UniformState : public State
{
public:
  UniformState(ref_ptr<Uniform> &uniform);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
  virtual void configureShader(ShaderConfiguration *cfg);

  ref_ptr<Uniform>& uniform();
protected:
  ref_ptr<Uniform> uniform_;
};

#endif /* UNIFORM_NODE_H_ */
