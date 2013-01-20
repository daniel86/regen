/*
 * shader-input-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_STATE_H_
#define SHADER_INPUT_STATE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/vbo.h>

typedef list< ref_ptr<ShaderInput> >::const_iterator ShaderInputIteratorConst;

/**
 * Provides vertex attributes.
 */
class ShaderInputState : public State
{
public:
  ShaderInputState();
  ShaderInputState(const ref_ptr<ShaderInput> &in);

  /**
   * Auto add to VBO when setInput() is called ?
   */
  void set_useVBOManager(GLboolean v);

  /**
   * vertex attributes.
   */
  list< ref_ptr<ShaderInput> >* inputsPtr();
  /**
   * vertex attributes.
   */
  const list< ref_ptr<ShaderInput> >& inputs() const;

  /**
   * Returns true if an attribute with given name was added.
   */
  GLboolean hasInput(const string &name) const;

  /**
   * Get attribute with specified name.
   */
  ShaderInputIteratorConst getInput(const string &name) const;

  ref_ptr<ShaderInput> getInputPtr(const string &name);

  /**
   * Set a vertex attribute.
   * uploadAttributes() must be called before the attributes are
   * uploaded to a VBO.
   */
  virtual ShaderInputIteratorConst setInput(const ref_ptr<ShaderInput> &in);

  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);

protected:
  list< ref_ptr<ShaderInput> > inputs_;
  set<string> inputMap_;
  GLboolean useVBOManager_;

  void removeInput( const string &name );
  virtual void removeInput(const ref_ptr<ShaderInput> &att);
};

#endif /* ATTRIBUTE_STATE_H_ */
