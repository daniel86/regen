/*
 * state.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_H_
#define STATE_H_

#include <set>

#include <ogle/utility/callable.h>
#include <ogle/utility/event-object.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/states/render-state.h>
#include <ogle/shader/shader-configuration.h>
#include <ogle/gl-types/uniform.h>

class State : public EventObject
{
public:
  State();

  void joinStates(ref_ptr<Uniform> uniform);
  void joinStates(ref_ptr<State> &state);

  void disjoinStates(ref_ptr<Uniform> uniform);
  void disjoinStates(ref_ptr<State> &state);

  void addEnabler(ref_ptr<Callable> &enabler);
  void addDisabler(ref_ptr<Callable> &disabler);
  void removeEnabler(ref_ptr<Callable> &enabler);
  void removeDisabler(ref_ptr<Callable> &disabler);

  virtual void configureShader(ShaderConfiguration*);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  list< ref_ptr<State> > joinned_;
  list< ref_ptr<Callable> > enabler_;
  list< ref_ptr<Callable> > disabler_;
};

#endif /* STATE_H_ */
