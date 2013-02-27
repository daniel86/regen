/*
 * toggle-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __TOGGLE_STATE__
#define __TOGGLE_STATE__

#include <ogle/states/state.h>
#include <ogle/states/render-state.h>

class ToggleState : public State
{
public:
  ToggleState(RenderState::Toggle key, GLboolean toggle);

  RenderState::Toggle key() const;
  GLboolean toggle() const;

  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  RenderState::Toggle key_;
  GLboolean toggle_;
};

#endif /* __TOGGLE_STATE__ */
