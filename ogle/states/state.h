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
#include <ogle/utility/gl-error.h>
#include <ogle/gl-types/shader-input.h>

class RenderState;

/**
 * Base class for states.
 * States can be enabled,disabled and updated.
 * Also you can join states together,
 * then the joined state will be enabled after
 * the original state.
 */
class State : public EventObject
{
public:
  State();
  virtual ~State() {}

  GLboolean isHidden() const;
  void set_isHidden(GLboolean);

  const list< ref_ptr<State> >& joined() const;

  void joinShaderInput(const ref_ptr<ShaderInput> &in);
  void disjoinShaderInput(const ref_ptr<ShaderInput> &in);

  void joinStates(const ref_ptr<State> &state);
  void joinStatesFront(const ref_ptr<State> &state);
  void disjoinStates(const ref_ptr<State> &state);

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
};

class StateSequence : public State
{
public:
  StateSequence();
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
};

#endif /* STATE_H_ */
