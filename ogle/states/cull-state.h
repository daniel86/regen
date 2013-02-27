/*
 * cull-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __CULL_FACE_STATE_H_
#define __CULL_FACE_STATE_H_

#include <ogle/states/state.h>

/**
 * Turn on front face culling (default is backface culling).
 */
class CullFrontFaceState : public State
{
public:
  CullFrontFaceState();
  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};

#endif /* __CULL_FACE_STATE_H_ */
