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
#include <ogle/utility/gl-util.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/render-state.h>

namespace ogle {

/**
 * Simple Interface for resizable states.
 */
class Resizable {
public:
  Resizable() {}
  virtual ~Resizable() {}
  virtual void resize()=0;
};

/**
 * Base class for states.
 * Joined states are enabled one after each other
 * and then disabled in reverse order.
 */
class State : public EventObject
{
public:
  State();
  virtual ~State() {}

  GLboolean isHidden() const;
  void set_isHidden(GLboolean);

  const list< ref_ptr<State> >& joined() const;

  void joinShaderInput(const ref_ptr<ShaderInput> &in, const string &name="");
  void disjoinShaderInput(const ref_ptr<ShaderInput> &in);

  void joinStates(const ref_ptr<State> &state);
  void joinStatesFront(const ref_ptr<State> &state);
  void disjoinStates(const ref_ptr<State> &state);

  GLuint shaderVersion() const;
  void setShaderVersion(GLuint version);

  void shaderDefine(const string &name, const string &value);
  const map<string,string>& shaderDefines() const;

  void shaderFunction(const string &name, const string &value);
  const map<string,string>& shaderFunctions() const;

  /**
   * For all joined states and this state collect all
   * uniform states and set the constant.
   */
  void setConstantUniforms(GLboolean isConstant=GL_TRUE);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  map<string,string> shaderDefines_;
  map<string,string> shaderFunctions_;

  list< ref_ptr<State> > joined_;
  GLboolean isHidden_;
  GLuint shaderVersion_;
};

/**
 * Joined states are enabled and disabled
 * one after each other.
 */
class StateSequence : public State
{
public:
  StateSequence();

  void set_globalState(const ref_ptr<State> &globalState);
  const ref_ptr<State>& globalState();

  // override
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  ref_ptr<State> globalState_;
};

} // end ogle namespace

#endif /* STATE_H_ */
