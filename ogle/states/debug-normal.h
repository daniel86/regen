/*
 * debug-normal.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef DEBUG_NORMAL_H_
#define DEBUG_NORMAL_H_

#include <ogle/states/shader-state.h>

/**
 * For child models only the normals are rendered.
 * Transform feedback is used so that the transformations
 * must not be done again.
 * You have to add 'Position' and 'nor' as transform
 * feedback attribute for children.
 */
class DebugNormal : public ShaderState
{
public:
  DebugNormal(
      GeometryShaderInput inputPrimitive,
      GLfloat normalLength=0.1);
  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
  virtual string name();
};

#endif /* DEBUG_NORMAL_H_ */
