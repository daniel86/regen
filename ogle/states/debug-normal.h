/*
 * debug-normal-state.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef DEBUG_NORMAL_H_
#define DEBUG_NORMAL_H_

#include <ogle/states/shader-state.h>

class DebugNormalState : public ShaderState
{
public:
  DebugNormalState(
      GeometryShaderInput inputPrimitive,
      GLfloat normalLength=0.1);

  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};

#endif /* DEBUG_NORMAL_H_ */
