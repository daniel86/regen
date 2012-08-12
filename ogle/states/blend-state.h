/*
 * blend-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __BLEND_STATE_H_
#define __BLEND_STATE_H_

#include <ogle/states/state.h>

class BlendState : public State
{
public:
  BlendState(
    GLenum sfactor=GL_SRC_ALPHA,
    GLenum dfactor=GL_ONE_MINUS_SRC_ALPHA);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
  virtual string name();
protected:
  GLenum sfactor_;
  GLenum dfactor_;
};

#endif /* __BLEND_STATE_H_ */
