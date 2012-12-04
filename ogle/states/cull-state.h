/*
 * cull-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __CULL_FACE_STATE_H_
#define __CULL_FACE_STATE_H_

#include <ogle/states/state.h>

class CullEnableState : public State
{
public:
  CullEnableState();
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  GLboolean culled_;
};
class CullDisableState : public State
{
public:
  CullDisableState();
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};
class CullFrontFaceState : public State
{
public:
  CullFrontFaceState();
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
};

#endif /* __CULL_FACE_STATE_H_ */
