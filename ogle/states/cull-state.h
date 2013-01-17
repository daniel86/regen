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
 * Enables face culling.
 */
class CullEnableState : public State
{
public:
  CullEnableState();
  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  GLboolean culled_;
};
/**
 * Disables face culling.
 */
class CullDisableState : public State
{
public:
  CullDisableState();
  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};
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
